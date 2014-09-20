<?hh
function l() {
  global $app;
  $args = func_get_args();
  $output = array();
  ob_start();
  foreach ($args as $arg) {
    if (!(is_string($arg) || is_numeric($arg))) {
      var_dump($arg);
      $output[] = ob_get_contents();
    } else {
      $output[] = $arg;
    }
  }
  ob_end_clean();
  $message = implode(' ', $output) . PHP_EOL;
  $backtrace = debug_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);
  $backtrace = array_shift($backtrace);
  $resource = fopen($_ENV['BASE_LOG_FILE'], 'a+w');
  // $log = sprintf('[%s %s:%d] %s', date('r'), $backtrace['file'], $backtrace['line'], $log);
  $log = sprintf('[%s:%d] %s',
    $backtrace['file'], $backtrace['line'], $message);

  fwrite($resource, $log);
  // file_put_contents($_ENV['BASE_LOG_FILE'], $log, FILE_APPEND);
}

function idx($array, $key, $default = null) {
  if (is_array($array)) {
    return array_key_exists($key, $array) ? $array[$key] : $default;
  } elseif (is_object($array)) {
    return isset($array->$key) ? $array->$key : $default;
  } else {
    return $default;
  }
}

function mid($id = null) {
  return is_a($id, 'MongoId') ? $id : new MongoId($id);
}

function mdate($date = null) {
  $mdate = $date == null ? time() : strtotime($date);
  return is_a($date, 'MongoDate') ? $date : new MongoDate($mdate);
}

function sha256($data) {
  return hash('sha256', $data);
}

function s() {
  $args = func_get_args();
  return call_user_func_array('sprintf', $args);
}

function regex(string $pattern, string $subject) {
  $out = [];
  if (preg_match($pattern, $subject, $out) === false) {
    return null;
  }

  return $out;
}

function regex_all(string $pattern, string $subject, ?int &$matches = null) {
  $out = [];
  $m = preg_match_all($pattern, $subject, $out);
  if ($m === false) {
    return null;
  }

  $matches = $m;
  return $out;
}

if (!function_exists('getallheaders')) {
  function getallheaders() {
    $headers = '';
    foreach ($_SERVER as $name => $value) {
     if (substr($name, 0, 5) == 'HTTP_') {
       $headers[str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($name, 5)))))] = $value;
     }
    }
    return $headers;
  }
}
