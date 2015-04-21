<?php

/**
 * @file
 * Contains classes for the Plugnpay Payment module, a payment method controller that works with Payment.
 */

require_once('plugnpay.class.inc');
/**
 * Plugnpay payment method controller.
 */
class PlugnpayPaymentMethodController extends PaymentMethodController {
  public $payment_method_configuration_form_elements_callback = 'plugnpay_payment_method_configuration';

  /**
   * Class constructor.
   */
  function __construct( $args = array() ) {
    $this->title = t('Plugnpay');
    $this->description = t("Plugnpay payment method.");

    if ( isset( $args['order_id'] ) ) {
      $this->orderID = $args['order_id'];
    }
  }

  /**
   * Implements PaymentMethodController::execute().
   */
  public function execute(Payment $payment) {

    $controller_data = $payment->method->controller_data;

    $card_name = $payment->context_data['card-name'];
    $card_number = $payment->context_data['card-number'];
    $card_exp = $payment->context_data['card-exp'];
    $cvv = $payment->context_data['card-cvv'];
    $amount = 0;

    foreach ($payment->line_items as $key => $data) {
      $temp_amount = $data->amount_total;
      if (NULL !== $temp_amount && is_numeric($temp_amount) && $temp_amount > 0) {
        $amount += (float) $temp_amount;
      }
    }

    $pnp = new plugnpay();

    //@ get details from the payment method
    $method = $payment->method;

    //@todo publisher credentials need to be encrypted in the db
    $pnp->apiurl = $method->controller_data['plugnpay_payment_server'];
    $pnp->publisher_name = $method->controller_data['plugnpay_account_name'];
    //$pnp->publisher_password = $method->controller_data['plugnpay_account_password'];
    $pnp->mode = $method->controller_data['plugnpay_mode'];
    $pnp->txn_type = 'auth'; // TODO from our settings
    $pnp->server_ip_address = $_SERVER['REMOTE_ADDR'];

    /*
      $order_id is our tempid
      WAS:      $acct-code2 is d for donation and r for registration
      WAS:      $acct-code3 is o for online, k for kiosk, a for admin
     */

    $pnp_response_object = $this->process_pnp_payment($pnp, $card_name, $card_number, $card_exp, $cvv, $amount, '', '', '');

    // get orderID, FinalStatus, sresp (short response), MErrMsg (error code), Errdetails (pipe-delimited text)
    // use status_from_response(), passing it the payment and the response code
    $response_details = $this->status_from_response($pnp_response_object);

    $final_status = array_key_exists('FinalStatus', $response_details) ? $response_details['FinalStatus']: '';
    $this_status = $this->status_translate($final_status);
    $payment->setStatus(new PaymentStatusItem($this_status));

    if ($payment->getStatus()->status == PAYMENT_STATUS_SUCCESS) {

      //@todo - the CC# and CVV are being cleaned in the example module too, don't do it twice
      // instead we need a setting that allows site admins to decide whether to store the data or not
      // if we store it we have to encrypt it

      //: clear sensitive cc details
      $new_card_number = '';
      for ($i = 0; $i < strlen($card_number) - 4; $i++ ) {
        $new_card_number .= 'X';
        if ($i % 4 == 0) {
          $new_card_number .= '-';
        }
      }
      $new_card_number .= substr($card_number, -4);
      $new_cvv = '';
      for ($i = 0; $i < strlen($cvv); $i++ ) {
        $new_cvv .= 'X';
      }
      $payment->context_data['card-number'] = $new_card_number;
      $payment->context_data['card-cvv'] = $new_cvv;

    }

    //@ set details in context_data
    foreach ($response_details as $key => $data) {
      $payment->context_data[$key] = $data;
    }
    entity_save('payment', $payment);

  }

  private function process_pnp_payment($pnp, $cardholdername, $cardnumber, $cardexp, $cardcvv, $amount,
                                       $order_id = '', $acct_code2 = '', $acct_code3 = '') {

    // process a payment
    $mode = $pnp->mode;
    $response = $pnp->auth($mode, array(
      'card_number' => $cardnumber,
      'card-name' => $cardholdername,
      'card-amount' => $amount,
      'card-exp' => $cardexp,
      'card-cvv' => $cardcvv,
      'acct-code2' => $acct_code2,
      'acct-code3' => $acct_code3,
      'order-id' => $order_id,
    ));

    return $response;

  }

  private function status_from_response($response_object) {

    // parse the response string
    // set response codes in our payment, and save it

    $response_details = (array)$response_object;

    /*
      FinalStatus	success/badcard/problem/fraud
          success - Transaction was successful
          badcard - Transaction failed due to decline from processor
          problem - Transaction failed due to problem unrelated to credit card.
          fraud - Transaction failed due to failure of FraudTrack2 filter settings.
    */

    /*
      resp-code	Alphanumeric Response Code - See Appendix.

      sresp	Simplified Response Code.
        A - Approved.
        C - Call Auth Center.
        D - Declined.
        P - Pick up card.
        X - Expired.
        E - Other Error.
    */
    if (array_key_exists('sresp', $response_details)) {
      switch (strtoupper($response_details['sresp'])) {
        case 'A':
          $response_details['sresp_detail'] = 'Approved';
          break;
        case 'C':
          $response_details['sresp_detail'] = 'Call Auth Center';
          break;
        case 'D':
          $response_details['sresp_detail'] = 'Declined';
          break;
        case 'P':
          $response_details['sresp_detail'] = 'Pick up card';
          break;
        case 'X':
          $response_details['sresp_detail'] = 'Expired';
          break;
        case 'E':
          $response_details['sresp_detail'] = 'Other Error';
          break;
      }
    }

  /*
    auth-code	Authorization Code.

    MErrMsg	Error Response Message.

    Errdetails	If transaction fails data validation or fraud testing, then message will comprise a pipe delimited list of problem variables and corresponding error messages. Requires subscription to FraudTrack2.

    aux-msg	Additional Message.

    orderID	Unique numeric order ID used to identify transaction for any future activity including voids and returns.

   */

    return $response_details;
  }

  /**
   * Implements PaymentMethodController::validate().
   */
  public function validate(Payment $payment, PaymentMethod $payment_method, $strict) {

    $payment->context_data['sessionid'] = session_id();

    // This payment method controller does not have a minimum amount, so when
    // the parent throws such an exception, catch it and do nothing.
    try {
      //@todo: check payment amount against the minimum that is set for this payment method

      //@todo: make sure we have all credit card info
    }
    catch (PaymentValidationAmountBelowMinimumException $e) {

    }

  }

  /**
   * @todo function documentation.
   */
  public function form(Payment $payment, $form, &$form_state) {
    global $base_url;

    $controller_data = $payment->method->controller_data;

    // Get settings from controller data.
    $mode = isset($controller_data['plugnpay_mode']) ? $controller_data['plugnpay_mode'] : '';
    $publisher_name = $controller_data['plugnpay_account_name'];
    $publisher_password = $controller_data['plugnpay_account_password'];
    $txn_type = array_key_exists('txn_type', $controller_data) ? $controller_data['txn_type'] : 'auth'; // 'auth'
    $url = $controller_data['plugnpay_payment_server'];

    /**
     * Calculate amount to pay.
     */
    $amount = 0;
    foreach ($payment->line_items as $line_item) {
      $amount += (1 + $line_item->tax_rate) * $line_item->amount
              * $line_item->quantity;
    }

    $payment_data = array(
      'amount' => $amount,
      'description' => $payment->description,
    );

    $payment_data['card-number'] = array_key_exists('card-number', $payment->context_data)
      ? $payment->context_data['card-number'] : '';
    $payment_data['card-name'] = array_key_exists('card-name', $payment->context_data)
      ? $payment->context_data['card-name'] : '';
    $payment_data['card-exp'] = array_key_exists('card-exp', $payment->context_data)
      ? $payment->context_data['card-exp'] : '';
    $payment_data['card-cvv'] = array_key_exists('card-cvv', $payment->context_data)
      ? $payment->context_data['card-cvv'] : '';

    $payment_data['card-code2'] = array_key_exists('card-code2', $payment->context_data)
      ? $payment->context_data['card-code2'] : '';
    $payment_data['card-code3'] = array_key_exists('card-code3', $payment->context_data)
      ? $payment->context_data['card-code3'] : '';


    $data = array(
      'pid' => $payment->pid,

/*      'card-number' => $payment_data['card-number'],
 *      'card-name' => $payment_data['card-name'],
 *      'card-exp' => $payment_data['card-exp'],
 *      'card-cvv' => $payment_data['card-cvv'],
 *      'card-code2' => $payment_data['card-code2'],
 *      'card-code3' => $payment_data['card-code3'],
*/
      'orderID' => $payment->pid,
      'order-id' => $payment->pid,
      'card-amount' => number_format($payment_data['amount'], 2, '.', ''),
    );

    foreach ($data as $name => $value) {
      if ( !empty($value)) {
        $form[$name] = array('#type' => 'hidden', '#value' => $value);
      }
    }
    $form['actions'] = array('#type' => 'actions');
    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Pay now'),
    );

    return $form;
  }

  /**
   * @todo Write function documentation.
   */
  public function form_validate($form, &$form_state) {

  }

  /**
   * @todo Write function documentation.
   */
  public function form_submit($form, &$form_state) {

  }

  /**
   * @todo Write function documentation.
   */
  private function status_translate($plugnpay_finalstatus) {

    $payment_status = PAYMENT_STATUS_PENDING;

    switch ($plugnpay_finalstatus) {
      case 'success':
        $payment_status = PAYMENT_STATUS_SUCCESS;
        break;
      case 'badcard':
      case 'problem':
      case 'fraud':
        $payment_status = PAYMENT_STATUS_FAILED;
      break;
    }

    return $payment_status;
  }

} // end controller
