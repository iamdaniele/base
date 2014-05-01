<?php

if (!array_key_exists('BASE_LOG_FILE', $_ENV)) {
  $_ENV['BASE_LOG_FILE'] = 'php://stderr';
}

$_ENV['BASE_LOG_FILE'] = str_replace(
  '{{PORT}}',
  $_ENV['PORT'],
  $_ENV['BASE_LOG_FILE']);

require_once 'common.php';
require_once 'BaseParam.php';
require_once 'BaseStorage.php';
require_once 'Base.php';

Base::registerAutoloader();

$app = new ApiRunner($map);
$app->run();
