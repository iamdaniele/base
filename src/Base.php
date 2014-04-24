<?php
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
        $r = new BaseHTMLView();
        $r->status(404);
        $r->render('Not Found');
      }

    } else {
      if (method_exists($this, 'renderError')) {
        $this->renderError($e);
      } else {
        $r = new BaseHTMLView();
        $r->status(404);
        $r->render('Not Found');
      }
    }
  }

  // Called when the controller executes getFlow with success. Override this
  // method when you need to send extra field in your response.
  protected function render() {echo '';}
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

class BaseHTMLView extends BaseView {
  public function render($out) {echo $out;}
}

class BaseLayout extends BaseView {
  protected
    $layout,
    $sections;

  public function __construct($layout = null) {
    if (!file_exists('layouts/' . $layout . '.html')) {
      throw new Exception('Layout does not exist: ' . $layout);
      return;
    }

    $this->layout = file_get_contents('layouts/' . $layout . '.html');
    $this->sections = [];
    preg_match_all('/{{[\w\d]+}}/', $this->layout, $this->sections);

    foreach ($this->sections[0] as &$section) {
      $section = strtolower($section);
    }

    if ($this->sections[0]) {
      $this->sections = array_flip($this->sections[0]);
    }

  }

  public function __call($name, $args) {
    $section = [];
    if (!preg_match('/addto([\w\d]+)/i', $name, $section)) {
      $section = str_ireplace('addto', '', name);
      throw new Exception('Invalid section: ' . $section);
      return $this;
    } else {
      $section = strtolower(array_pop($section));
    }

    if (!idx($this->sections, '{{' . $section . '}}')) {
      throw new Exception('Invalid section: ' . $section);
      return $this;
    }

    if (!($args[0] instanceof BaseWidget)) {
      throw new Exception('Invalid widget provided');
      return $this;
    }
    return $this;
  }

  public function render($out) {
    echo $out;
  }
}

abstract class BaseWidget {
  protected $partials;
  public function __construct() {
    $this->partials = $this->partials();
    if (!is_array($this->partials)) {
      throw new RuntimeException('Invalid partials provided.');
      return;
    }
  }

  public function partials() {
    return [];
  }

  abstract public function render();
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

class BaseNotFoundController extends BaseController {
  protected function params() {
    return [
      BaseParam::StringType('path_info')
    ];
  }
  public function render() {
    $r = new BaseHTMLView();
    $r->status(404);
    $r->render('Not found: ' . $this->param('path_info'));
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

    unset($this->document[$key]);
  }

  public function document() {return $this->document;}
  public function requiredFields() {return [];}
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
    } else {
      l('no mongoHQ url');
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
