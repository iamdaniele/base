<?php

if (!array_key_exists('BASE_LOG_FILE', $_ENV)) {
  $_ENV['BASE_LOG_FILE'] = 'php://stderr';
}

if (array_key_exists('APPLICATION_ENV', $_ENV) &&
  $_ENV['APPLICATION_ENV'] == 'prod') {
  $_ENV['BASE_LOG_FILE'] = '/tmp/heroku.apache2_error.' .
    $_SERVER['PORT'] . '.log';
}

require_once 'common.php';
require_once 'BaseParam.php';
require_once 'BaseStorage.php';
require_once 'Base.php';

Base::registerAutoloader();

$app = new ApiRunner($map);
$app->run();
