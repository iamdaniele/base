<?hh

if (!array_key_exists('BASE_LOG_FILE', $_ENV)) {
  $_ENV['BASE_LOG_FILE'] = 'php://stderr';
}

$_ENV['BASE_LOG_FILE'] = str_replace(
  '{{PORT}}',
  $_ENV['PORT'],
  $_ENV['BASE_LOG_FILE']);

require_once 'common.hh';
require_once 'BaseParam.hh';
require_once 'BaseStore.hh';
require_once 'Base.hh';

Base::registerAutoloader();

$app = new ApiRunner($map);
$app->run();
