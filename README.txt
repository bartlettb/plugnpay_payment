CONTENTS OF THIS FILE
---------------------
 * Introduction
 * Requirements
 * Installation
 * Configuration
 * Maintainers

INTRODUCTION
------------
This module provides Plugnpay (http://www.plugnpay.com/) integration for the
Payment platform (https://drupal.org/project/payment). Plugnpay is a payment
service provider. With this module enabled site owner can enable customers
(more general visitors) to perform payments to the site owners Plugnpay account.
Customers can pay using a credit card (http://www.plugnpay.com/about.php).

REQUIREMENTS
------------
This module requires the following modules:
 * Payment (https://drupal.org/project/payment)
 * Entity API (https://www.drupal.org/project/entity)

INSTALLATION
------------
 * Install as you would normally install a contributed drupal module. See:
   https://drupal.org/documentation/install/modules-themes/modules-7
   for further information.

CONFIGURATION
-------------
After installation a payment method for Plugnpay should be created in the
configuration of the Payment platform.
 * Go to admin/config/services/payment
 * Click on Payment methods
 * Click on Add payment method
 * Click on Plugnpay in the list of available payment method types
 * Fill in a title and the Plugnpay API details (which can be found in the dashboard
   of your Plugnpay account)
 * Click the Save button

 Note: Make sure your Plugnpay settings on the Plugnpay site allow your web server
 to contact the Plugnpay server. For example the IP address of the web server using
 this module might have to be in the list of allowed IP addresses according to Plugnpay.

 Log in to the Plugnpay web site (https://pay1.plugnpay.com/admin/security.cgi) and
 check Security Administration.

MAINTAINERS
-----------
Current maintainers:
 * Rebecca Bartlett (bart atlas) - https://www.drupal.org/user/1243726

This project has been sponsored by:
 * Atlas Geek Services LLC
   http://www.atlasgeekservices.com
 * Bermuda End-to-End
   The largest charity event in Bermuda. Raising funds to help island charities.
   Visit http://www.bermudaendtoend.bm for more information.
