<?hh
class BaseController {
  protected
    $app,
    $params,
    $restricted,
    $path,
    $skipParamValidation;

  public final function __construct(
    $route = null,
    $params = [],
    $files = [],
    $can_access_restricted_endpoints = false) {
    $this->restricted = false;
    $this->params = [];
    $this->path = $route;

    $this->skipParamValidation = false;

    try {
      $params = $this->params();
      if (true === $this->skipParamValidation) {
        $this->params = array_merge($_GET, $_POST, $_REQUEST, $_FILES);
      } elseif (is_array($params)) {
        foreach ($params as $param) {
          $this->params[$param->name()] = $param;
        }
      } else {
        throw new RuntimeException('Invalid params supplied.');
      }

      $this->init();
      $this->genFlow();
      $this->success = true;
      $this->out();

    } catch (Exception $e) {
      $this->success = false;
      $this->outError($e);
      return $this;
    }

    return $this;
  }

  protected function params() {
    return [];
  }

  protected function skipParamValidation() {$this->skipParamValidation = true;}

  // Controllers can override this and use it as a constructor.
  protected function init() {}

  // Controllers need to implement this in order to generate their flow.
  // Yielding a strict false will stop the controller and generate an
  // exception, which is useful to halt the execution flow when an error
  // occurs. An optional $error can be specified if you need to set the
  // exception message.
  protected function genFlow(&$error = null) {return [];}

  protected function isXHR() {
    return !!idx($_SERVER, 'HTTP_X_REQUESTED_WITH') == 'XMLHTTPRequest';
  }

  protected function status(int $status): void {
    if ($status == 404) {
      header('HTTP/1.0 404 Not Found');
    } else {
      header('HTTP/1.0 ' . $status);
    }
  }

  private function out() {
    if ($this->isXHR()) {
      $this->renderJSON();
    } else {
      $this->render();
    }
  }

  private function outError(Exception $e) {
    if ($this->isXHR()) {
      if (method_exists($this, 'renderJSONError')) {
        $this->renderJSONError($e);
      } else {
        $this->status(404);
        echo <h1>Not Found</h1>;
      }

    } else {
      if (method_exists($this, 'renderError')) {
        $this->renderError($e);
      } else {
        $this->status(404);
        echo <h1>Not Found</h1>;
      }
    }
  }

  // Called when the controller executes getFlow with success. Override this
  // method when you need to send extra field in your response.
  // protected function render() {echo '';}
  protected function renderJSON() {echo json_encode([]);}

  protected function renderError(Exception $e) {var_dump($e);}
  protected function renderJSONError(Exception $e) {echo json_encode($e);}

  protected final function param($key) {
    return idx($this->params, $key) ? $this->params[$key]->value() : null;
  }

  protected final function env($key) {
    return idx($_SERVER, $key, null);
  }

  public final function done() {
    return $this->success;
  }

  protected final function redirect(URL $url) {
    header('Location: ' . $url->__toString());
    die;
  }
}

abstract class BaseView {
  protected $status;

  public function __construct() {
    $this->status = 200;
  }

  public function status($code = 200) {
    header(' ', true, $code);
  }

  abstract public function render($out);
}

abstract class BaseMutatorController extends BaseController {
  abstract public function render();

  public function renderError(?Exception $e): URL {}
}

class BaseJSONView extends BaseView {
  protected final function success($data = null, $http_status = 200) {
    $this->status($http_status);
    $payload = ['success' => true];

    if ($data) {
      $payload['data'] = $data;
    }

    $this->render($payload);
  }

  protected final function error($code = true) {
    $this->status($http_status);
    $this->render(['success' => false, 'error' => $code]);
  }

  public final function render($msg) {
    // if (!headers_sent()) {
      header('Access-Control-Allow-Origin: *');
      header('Content-type: application/json; charset: utf-8');

      if ($http_status !== null) {
        header(' ', true, $this->status);
      }

    // }
    echo json_encode($msg);
  }
}

class URL {
  protected array $url;
  protected array $query;
  public function __construct(string $url) {
    $this->url = parse_url($url);
    if (idx($this->url, 'query')) {
      $this->query = parse_str($this->url['query']);
    }
  }

  public function query(string $key, ?mixed $value) {
    if ($value === null) {
      return idx($this->query, $key, null);
    }

    $this->query[$key] = $value;
    return $this;
  }

  public function hash(?string $hash) {
    if ($hash === null) {
      return idx($this->url, 'hash', null);
    }

    $this->url['hash'] = $hash;
    return $this;
  }

  public function port(?int $port) {
    if ($port === null) {
      return idx($this->url, 'port', null);
    }

    $this->url['port'] = $port;
    return $this;
  }

  public function user(?string $user) {
    if ($user === null) {
      return idx($this->url, 'user', null);
    }

    $this->url['user'] = $user;
    return $this;
  }

  public function pass(?string $pass) {
    if ($pass === null) {
      return idx($this->url, 'pass', null);
    }

    $this->url['pass'] = $pass;
    return $this;
  }

  public function host(?string $host) {
    if ($host === null) {
      return idx($this->url, 'host', null);
    }

    $this->url['host'] = $host;
    return $this;
  }

  public function path(?string $path) {
    if ($path === null) {
      return idx($this->url, 'path', null);
    }

    $this->url['path'] = $path;
    return $this;
  }

  public function __toString() {
    $query = '';
    if (!empty($this->query)) {
      $query = '?' . http_build_query($this->query);
    }

    $path = '';
    if (idx($this->url, 'path')) {
      $path = $this->url['path'][0] == '/' ?
        $this->url['path'] :
        '/' . $this->url['path'];
    }

    return sprintf(
      '%s%s%s%s%s%s%s%s%s%s%s',
      idx($this->url, 'scheme'),
      idx($this->url, 'scheme') ? '://' : '',
      idx($this->url, 'user') ? $this->url['user'] : '',
      idx($this->url, 'user') && idx($this->url, 'pass') ? ':' : '',
      idx($this->url, 'pass') ? $this->url['pass'] : '',
      idx($this->url, 'user') || idx($this->url, 'pass') ? '@' : '',
      idx($this->url, 'host') ? $this->url['host'] : '',
      idx($this->url, 'port') ? ':' . $this->url['port'] : '',
      $path,
      $query,
      idx($this->url, 'hash') ? '#' . $this->url['hash'] : '',
    );
  }
}

class BaseNotFoundController extends BaseController {
  protected function params() {
    return [
      BaseParam::StringType('path_info')
    ];
  }
  public function render() {
    $this->status(404);
    echo <h1>Not Found: {$this->param('path_info')}</h1>;
  }
}

class ApiRunner {
  protected
    $map,
    $pathInfo,
    $params,
    $paramNames;

  public function __construct($map) {
    $this->map = $map;
    $this->paramNames = [];
    $this->params = [];
  }

  protected function getPathInfo() {
    if ($this->pathInfo) {
      return $this->pathInfo;
    }

    if (strpos($_SERVER['REQUEST_URI'], $_SERVER['SCRIPT_NAME']) === 0) {
        $script_name = $_SERVER['SCRIPT_NAME']; //Without URL rewrite
    } else {
        $script_name = str_replace('\\', '/', dirname($_SERVER['SCRIPT_NAME'])); //With URL rewrite
    }

    $this->pathInfo = substr_replace(
      $_SERVER['REQUEST_URI'], '', 0, strlen($script_name));
    if (strpos($this->pathInfo, '?') !== false) {
        $this->pathInfo = substr_replace(
          $this->pathInfo, '', strpos($this->pathInfo, '?'));
    }

    $script_name = rtrim($script_name, '/');
    $this->pathInfo = '/' . ltrim($this->pathInfo, '/');


    return $this->pathInfo;
  }

  public function matches($resourceUri, $pattern)
  {
      //Convert URL params into regex patterns, construct a regex for this route, init params
      $patternAsRegex = preg_replace_callback('#:([\w]+)\+?#', [$this, 'matchesCallback'],
          str_replace(')', ')?', (string) $pattern));

      if (substr($pattern, -1) === '/') {
          $patternAsRegex .= '?';
      }

      //Cache URL params' names and values if this route matches the current HTTP request
      if (!preg_match('#^' . $patternAsRegex . '$#', $resourceUri, $paramValues)) {
          return false;
      }
      foreach ($this->paramNames as $name) {
          if (isset($paramValues[$name])) {
              if (isset($this->paramNamesPath[ $name ])) {
                  $this->params[$name] = explode('/', urldecode($paramValues[$name]));
              } else {
                  $this->params[$name] = urldecode($paramValues[$name]);
              }
          }
      }

      return true;
  }

  /**
   * Convert a URL parameter (e.g. ":id", ":id+") into a regular expression
   * @param  array    URL parameters
   * @return string   Regular expression for URL parameter
   */
  protected function matchesCallback($m)
  {
      $this->paramNames[] = $m[1];
      if (isset($this->conditions[ $m[1] ])) {
          return '(?P<' . $m[1] . '>' . $this->conditions[ $m[1] ] . ')';
      }
      if (substr($m[0], -1) === '+') {
          $this->paramNamesPath[ $m[1] ] = 1;

          return '(?P<' . $m[1] . '>.+)';
      }

      return '(?P<' . $m[1] . '>[^/]+)';
  }

  protected function selectController() {
    foreach ($this->map as $route => $controller) {
      if ($this->matches($this->getPathInfo(), $route)) {
        return $controller;
      }
    }
    return false;
  }

  protected function getRequestMethod() {
    return $_SERVER['REQUEST_METHOD'];
  }

  protected function getParams() {
    return $this->params;
  }

  protected function getFiles() {
    return [];
  }

  protected function canAccessRestrictedEndpoints() {
    return false;
  }

  protected function notFound() {
    $_GET['path_info'] = $this->getPathInfo();
    $controller = new BaseNotFoundController();
    return $controller;
  }

  public function run() {
    $method = $this->getRequestMethod();
    $controller_path = $this->selectController();
    $is_mutator = false;

    if (false === $controller_path) {
      return $this->notFound();
    }

    $controller_name = array_pop(explode('/', $controller_path));
    switch ($method) {
      case 'GET':
          $controller_name .= 'Controller';
        break;
      case 'HEAD':
      case 'OPTIONS':
        $controller_name .= ucfirst(strtolower($method)) . 'Controller';
        $controller_path .= ucfirst(strtolower($method));
        $origin = idx(getallheaders(), 'Origin');
        $allow_headers = array_keys(getallheaders());
        $allow_headers[] = 'Access-Control-Allow-Origin';
        $headers = implode(', ', $allow_headers);
        header('Access-Control-Allow-Origin: *');
        header('Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS');
        header('Access-Control-Max-Age: 604800');
        header('Access-Control-Allow-Headers: ' . $headers);
        die;
        break;

      case 'POST':
      case 'PUT':
      case 'DELETE':
        $is_mutator = true;
        $controller_path = str_replace($controller_name, '', $controller_path);
        // $controller_path .= 'mutators/' . $controller_name .
        //   ucfirst(strtolower($method));

        $controller_path .= $controller_name . ucfirst(strtolower($method));
        $controller_name .= ucfirst(strtolower($method)) . 'Controller';
        break;
    }

    $controller_path = 'controllers/' . $controller_path . '.php';

    if (false === strpos($controller_name, 'Controller') ||
      !file_exists($controller_path)) {
      return $this->notFound();
    }

    require_once $controller_path;
    if ($is_mutator &&
      get_parent_class($controller_name) != 'BaseMutatorController') {
      throw new Exception(
        sprintf(
          '%s must be an instance of BaseMutatorController',
          $controller_name));
      return null;
    }

    $controller = new $controller_name(
      $this->getPathInfo(),
      $this->getParams(),
      $this->getFiles(),
      $this->canAccessRestrictedEndpoints());

    return $controller;
  }
}

abstract class BaseEnum {
  protected $value;
  /**
   * Allows to reference a BaseEnum's value by calling:
   * AVeryLongEnumName::MY_VALUE()
   */
  public static function __callStatic($name, $args) {
    $const = static::class . '::' . $name;
    if (!defined($const)) {
      throw new InvalidArgumentException(
        sprintf(
          'Could not find enumeration %s in %s',
          $name,
          static::class));

      return null;
    } else {
      return new static($name);
    }
  }

  public final function __construct($key = '__default') {
    $this->setValue($key);
  }

  protected final function setValue($key) {
    if (static::isValid($key)) {
      $this->value = constant(static::class . '::' . $key);
    } else {
      throw new InvalidArgumentException(
        sprintf(
          'Could not find enumeration %s in %s',
          $key,
          get_class($this)));
    }
  }

  protected static final function isValid($key) {
    return defined(static::class . '::' . $key);
  }

  public static final function validValues() {
    $r = new ReflectionClass(get_called_class());
    return array_keys($r->getConstants());
  }

  public final function value() {
    return $this->value;
  }

  public final function __toString() {
    return (string)$this->value;
  }
}

class Base {
  public static function registerAutoloader() {
    spl_autoload_register(function ($class) {
      $map = [
        'Provider' => 'providers',
        'Model' => 'models',
        'Store' => 'storage',
        'Trait' => 'traits',
        'Enum' => 'const',
        'Const' => 'const',
        'Exception' => 'exceptions',
      ];

      // These classes are stored in lib/base or lib/queue, so no need to
      // autoload
      switch($class) {
        case 'BaseModel':
        case 'BaseEnum':
        case 'BaseQueueStore':
        case 'BaseQueueFilesStore':
        case 'BaseQueueFileModel':
        case 'BaseStore':
        case 'BaseEnum':
        return;
      }

      $pattern = '/^(\w+)(' . implode('|', array_keys($map)) . ')$/';

      $class_type = [];
      if (preg_match($pattern, $class, $class_type)) {
        $dir = array_pop($class_type);
        require $map[$dir] . DIRECTORY_SEPARATOR . str_replace($dir, '', $class) . '.php';
      }

      $layout_type = [];
      if (preg_match('/^xhp_layout__([\d\w-]+)$/', $class, $layout_type)) {
        $layout = array_pop($layout_type);
        require 'layouts' . DIRECTORY_SEPARATOR . $layout . '.hh';
      }

      $widget_type = [];
      if (preg_match('/^xhp_widget__([\d\w-]+)$/', $class, $widget_type)) {
        $widget = array_pop($widget_type);
        require 'widgets' . DIRECTORY_SEPARATOR . $widget . '.hh';
      }

    });
  }
}

abstract class BaseModel {
  const
    FIELD = 'FIELD',
    REQUIRED = 'REQUIRED';

  protected
    $strict,
    $schema,
    $document;

  public function __construct($document = [], $strict = false) {
    $this->strict = !!$strict;
    $this->schema = $this->schema();

    $this->document = [];

    if (!is_array($this->schema)) {
      throw new Exception('Schema must be a key => value array.');
      return null;
    }

    $this->schema['_id'] = self::REQUIRED;

    if (is_array($document) && !empty($document)) {
      foreach ($document as $key => $value) {
        $this->set($key, $value);
      }
    }
  }

  abstract public function schema();

  public function __call($method, $args) {
    if (strpos($method, 'get') === 0) {
      $op = 'get';
    } elseif (strpos($method, 'set') === 0) {
      $op = 'set';
    } elseif (strpos($method, 'remove') === 0) {
      $op = 'remove';
    } elseif (strpos($method, 'has') === 0) {
      $op = 'has';
    } else {
      $e = sprintf('Method "%s" not found in %s', $method, get_called_class());
      throw new RuntimeException($e);
      return null;
    }

    $method = preg_replace('/^(get|set|remove|has)/i', '', $method);

    preg_match_all(
      '!([A-Z][A-Z0-9]*(?=$|[A-Z][a-z0-9])|[A-Za-z][a-z0-9]+)!',
      $method,
      $matches);

    $ret = $matches[0];
    foreach ($ret as &$match) {
      $match = $match == strtoupper($match) ?
        strtolower($match) :
        lcfirst($match);
    }

    $field = implode('_', $ret);
    if ($this->strict && !idx($this->schema, $field)) {
      $e = sprintf(
        '"%s" is not a valid field for %s',
        $field,
        get_called_class());
      throw new InvalidArgumentException($e);
      return null;
    }

    $arg = array_pop($args);

    if (!method_exists($this, $op)) {
      $e = sprintf('%s::%s() is not defined', get_called_class(), $op);
      throw new Exception($e);
    }

    return $op == 'set' ? $this->set($field, $arg) : $this->$op($field);
  }

  private function overriddenMethod($prefix, $key) {
    $method = $prefix . preg_replace_callback(
      '/(?:^|_)(.?)/',
      function($matches) {
        return strtolower($matches[0]);
      },
      $key);

    return method_exists($this, $method) ? strtolower($method) : null;
  }

  public function getID() {return idx($this->document, '_id');}
  public function setID($value) {$this->document['_id'] = $value;}
  public final function get($key) {
    if (!is_string($key) || empty($key)) {
      throw new InvalidArgumentException('Invalid null key');
      return null;
    }

    $method = $this->overriddenMethod(__FUNCTION__, $key);
    if ($method) {
      return $this->$method();
    }

    if ($this->strict && !array_key_exists($key, $this->document)) {
      $e = sprintf(
        '\'%s\' is not a valid field for %s',
        $field,
        get_called_class());
      throw new InvalidArgumentException($e);
      return null;
    }

    return idx($this->document, $key);
  }

  public final function set($key, $value) {
    if (!is_string($key) || empty($key)) {
      throw new InvalidArgumentException('Invalid null key');
      return null;
    }

    $method = $this->overriddenMethod(__FUNCTION__, $key);
    if ($method) {
      return $this->$method($value);
    }

    if ($this->strict && !idx($this->schema, $key)) {
      $e = sprintf(
        '%s is not a valid field for %s',
        $field,
        get_called_class());
      throw new InvalidArgumentException($e);
      return;
    }

    $this->document[$key] = $value;
  }

  public function has($key) {
    if (!is_string($key) || empty($key)) {
      throw new InvalidArgumentException('Invalid null key');
      return null;
    }

    $method = $this->overriddenMethod(__FUNCTION__, $key);
    if ($method) {
      return $this->$method();
    }

    return array_key_exists($key, $this->document);
  }

  public final function remove($key) {
    if (!is_string($key) || empty($key)) {
      throw new InvalidArgumentException('Invalid null key');
      return null;
    }

    $method = $this->overriddenMethod(__FUNCTION__, $key);
    if ($method) {
      return $this->$method();
    }

    if ($this->strict &&
      (idx($this->schema, $key) == self::REQUIRED || $key == '_id')) {
      $e = sprintf(
        'Cannot remove %s required field %s',
        get_called_class(),
        $field);
      throw new InvalidArgumentException($e);
      return null;
    }

    unset($this->document[$key]);
  }

  public function document() {return $this->document;}
}

class Error {
  const
    SESSION_EXPIRED_OR_INVALID_SESSION = 1,
    NO_ACCESS_TOKEN = 2,
    FBUSER_NO_ADACCOUNT = 3,
    FBREQUEST_INVALID_RESPONSE = 4,
    OUR_FAULT = 5,
    INVALID_PARAMS = 6;
}

class MongoInstance {
  protected static $db = [];
  public static function get($collection = null, $with_collection = false) {
    if (strpos('mongodb://', $collection) !== false) {
      $db_url = explode('/', $collection);
      $collection = $with_collection ? array_pop($collection) : null;
      $db_url = implode('/', $db_url);
    } elseif (isset($_SERVER['MONGOHQ_URL'])) {
      $db_url = $_SERVER['MONGOHQ_URL'];
      $parts = parse_url($db_url);
      if (idx($parts, 'user') && idx($parts, 'pass')) {
        $auth_pattern = sprintf('%s:%s@', $parts['user'], $parts['pass']);
        $db_url = str_replace($auth_pattern, '', $db_url);
      }
    } else {
      throw new Exception('No MONGOHQ_URL specified');
      return null;
    }

    if (idx(self::$db, $db_url)) {
      return $collection != null ?
        self::$db[$db_url]->selectCollection($collection) :
        self::$db[$db_url];
    }

    $dbname = array_pop(explode('/', $db_url));
    $db = new Mongo($db_url);
    self::$db[$db_url] = $db->selectDB($dbname);
    if ($parts && idx($parts, 'user') && idx($parts, 'pass')) {
      self::$db[$db_url]->authenticate($parts['user'], $parts['pass']);
    }

    if ($collection) {
      return $collection != null ?
        self::$db[$db_url]->selectCollection($collection) :
        self::$db[$db_url];
    }
  }
}

class MongoFn {
  public static function get($file, $scope = []) {
    $code = file_get_contents('mongo_functions/' . $file . '.js');
    return new MongoCode($code, $scope);
  }
}

abstract class :base:layout extends :x:element {
  public function init() {}

  public final function section(string $section): :x:frag {
    invariant(
      isset($this->sections),
      'At least one section need to be defined');

    invariant(
      idx($this->sections, $section) !== null,
      'Section %s does not exist', $section);

    return $this->sections[$section];
  }

  abstract public function render();
}

class :base:widget extends :x:element {
  public function stylesheets(): ?array<:link> {
    return null;
  }

  public function javascripts(): ?array<:script> {
    return null;
  }
}

class BaseLayoutHelper {
  protected static :x:frag $javascripts;
  protected static :x:frag $stylesheets;

  public static function addJavascript(:script $javascript): void {
    if (!self::$javascript) {
      self::$javascripts = <x:frag />;
    }

    self::$javascripts->appendChild($javascript);
  }

  public static function addStylesheet(:link $stylesheet): void {
      if (!self::$stylesheets) {
        self::$stylesheets = <x:frag />;
      }

      self::$stylesheets->appendChild($stylesheet);
  }

  public static function javascripts(): :x:frag {
    return self::$javascripts ? self::$javascripts : <x:frag />;
  }

  public static function stylesheets(): :x:frag {
    return self::$stylesheets ? self::$stylesheets : <x:frag />;
  }

}
