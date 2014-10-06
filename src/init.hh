<?hh
chdir(__DIR__);
chdir(realpath('../../../../'));

if (array_key_exists('APPLICATION_ENV', $_ENV) &&
  $_ENV['APPLICATION_ENV'] != 'prod') {
  $env_file = 'env/' . $_SERVER['SERVER_NAME'] . '.json';
  if (file_exists($env_file)) {
    $vars = json_decode(file_get_contents($env_file), true);
    if (json_last_error() == JSON_ERROR_NONE) {
      foreach ($vars as $key => $value) {
        $_ENV[$key] = $value;
      }
    }
  }
}

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
require_once 'BaseWorker.hh';
require_once 'Base.hh';

register_shutdown_function('fatal_log');
Base::registerAutoloader();
