<?hh
class BaseController {
  protected
    $app,
    $params,
    $restricted,
    $path,
    $skipParamValidation,
    $jsonForced;

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
        $this->params = array_merge($_GET, $_POST, $_FILES);
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

  protected function forceJSON() {
    $this->jsonForced = true;
  }

  private function isJSONForced() {
    return $this->jsonForced;
  }

  private function out() {
    try {
      if ($this instanceof BaseListener) {
        $this->render();
      } elseif ($this->isXHR() || $this->isJSONForced()) {
        $view = $this->renderJSON();
        $view->render();
      } else {
        $layout = $this->render();
        // Pre-render layout in order to trigger widgets' CSSs and JSs
        $layout->__toString();
        if (method_exists($layout, 'hasSection')) {
          if ($layout->hasSection('stylesheets')) {
            foreach (BaseLayoutHelper::stylesheets() as $css) {
              $layout->section('stylesheets')->appendChild($css);
            }
          }

          if ($layout->hasSection('javascripts')) {
            foreach (BaseLayoutHelper::javascripts() as $js) {
              $layout->section('javascripts')->appendChild($js);
            }
          }
        }

        echo $layout;
      }
    } catch (Exception $e) {
      l($e);
      die;
    }
  }

  private function outError(Exception $e) {
    if ($this instanceof BaseListener) {
      l(sprintf(
        '%s %s: %s (%s:%s)',
        get_class($this),
        get_class($e),
        $e->getMessage(),
        $e->getFile(),
        $e->getLine()));
      return;
    } elseif ($this->isXHR()) {
      if (method_exists($this, 'renderJSONError')) {
        $view = $this->renderJSONError($e);
        $view->render();
      } else {
        $this->status(404);
        echo <h1>Not Found</h1>;
      }

    } else {
      if (method_exists($this, 'renderError')) {
        $layout = $this->renderError($e);
        if (!$layout) {
          return;
        }
        // Pre-render layout in order to trigger widgets' CSSs and JSs
        $layout->__toString();
        if (method_exists($layout, 'hasSection')) {
          if ($layout->hasSection('stylesheets')) {
            foreach (BaseLayoutHelper::stylesheets() as $css) {
              $layout->section('stylesheets')->appendChild($css);
            }
          }

          if ($layout->hasSection('javascripts')) {
            foreach (BaseLayoutHelper::javascripts() as $js) {
              $layout->section('javascripts')->appendChild($js);
            }
          }
        }

        echo $layout;

      } else {
        $this->status(404);
        echo <h1>Not Found</h1>;
      }
    }
  }

  // Called when the controller executes getFlow with success. Override this
  // method when you need to send extra field in your response.
  // protected function render() {echo '';}
  protected function renderJSON(): BaseJSONView {
    $view = new BaseJSONView();
    $view->success();
    return $view;
  }

  protected function renderError(Exception $e) {
    return <div>{print_r($e, true)}</div>;
  }

  protected function renderJSONError(Exception $e): BaseJSONView {
    $view = new BaseJSONView();
    $view->error($e->getMessage(), 500, $e->getCode());
    return $view;
  }

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

  public function status(int $code = 200) {
    http_response_code($code);
  }

  abstract public function render(bool $return_instead_of_echo);
}

abstract class BaseMutatorController extends BaseController {
  abstract public function render();

  public function renderError(?Exception $e): URL {}
}

class BaseListener extends BaseController {
  final public function render() {}
  public function renderError(?Exception $e): URL {}
}

class BaseJSONView extends BaseView {
  private array<string, mixed> $_payload;
  public final function success(
    array<string, mixed> $data = null,
    int $http_status = 200): this {

    $this->status($http_status);
    $this->_payload = ['success' => true];

    if ($data) {
      $this->_payload['data'] = $data;
    }

    return $this;
  }

  public final function error(
    ?string $message,
    int $http_status = 500,
    int $code = -1): this {

    $this->status($http_status);
    $this->_payload = [
      'success' => false,
      'message' => $message,
      'code' => $code,
    ];

    return $this;
  }

  public final function render(bool $return_instead_of_echo = false) {
    if ($return_instead_of_echo) {
      return json_encode($this->_payload);
    } else {
      header('Access-Control-Allow-Origin: *');
      header('Content-type: application/json; charset: utf-8');
      echo json_encode($this->_payload);
    }
  }
}

class URL {
  protected array $url;
  protected array $query;
  public function __construct(?string $url = null) {
    $current_url = parse_url($this->buildCurrentURL());
    $parsed_url = [];
    if ($url !== null) {
      $parsed_url = parse_url($url);
      invariant($parsed_url !== false, 'Invalid URL');
    }

    $this->url = array_merge($current_url, $parsed_url);

    if (idx($this->url, 'query')) {
      parse_str($this->url['query'], $this->query);
    }
  }

  public static function route(string $name, array $params = []): URL {
    return BaseRouter::generateUrl($name, $params);
  }

  protected function buildCurrentURL(): string {
    return sprintf('%s://%s%s%s',
      'https',
      EnvProvider::get('SERVER_NAME'),
      EnvProvider::has('SERVER_PORT') &&
      EnvProvider::get('SERVER_PORT') != 443 &&
      EnvProvider::get('SERVER_PORT') != 80 ?
        ':' . EnvProvider::get('SERVER_PORT') :
        '',
      idx($_SERVER, 'REQUEST_URI'));
  }

  public function query(string $key, ?mixed $value = null) {
    if ($value === null) {
      return idx($this->query, $key, null);
    }

    $this->query[$key] = $value;
    return $this;
  }

  public function removeQuery(?string $key = null) {
    if ($key === null) {
      $this->query = [];
    } elseif (idx($this->query, $key)) {
      unset($this->query[$key]);
    }
    return $this;
  }

  public function hash(?string $hash = null) {
    if ($hash === null) {
      return idx($this->url, 'hash', null);
    }

    $this->url['hash'] = $hash;
    return $this;
  }

  public function port(?int $port = null) {
    if ($port === null) {
      return idx($this->url, 'port', null);
    }

    if ($port === 0) {
      unset($this->url['port']);
    } else {
      $this->url['port'] = $port;
    }

    return $this;
  }

  public function user(?string $user = null) {
    if ($user === null) {
      return idx($this->url, 'user', null);
    }

    $this->url['user'] = $user;
    return $this;
  }

  public function pass(?string $pass = null) {
    if ($pass === null) {
      return idx($this->url, 'pass', null);
    }

    $this->url['pass'] = $pass;
    return $this;
  }

  public function host(?string $host = null) {
    if ($host === null) {
      return idx($this->url, 'host', null);
    }

    $this->url['host'] = $host;
    return $this;
  }

  public function path(?string $path = null) {
    if ($path === null) {
      return idx($this->url, 'path', null);
    }

    $this->url['path'] = $path;
    return $this;
  }

  public function scheme(?string $scheme = null) {
    if ($scheme === null) {
      return idx($this->url, 'scheme', null);
    }

    $this->url['scheme'] = $scheme;
    return $this;
  }

  public function isAbsolute(): bool {
    return !!(idx($this->url, 'scheme') && idx($this->url, 'host'));
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
    return <h1>Not Found: {$this->param('path_info')}</h1>;
  }

  public function renderJSON(): BaseJSONView {
    $view = new BaseJSONView();
    $view->error('Invalid endpoint: ' . $this->param('path_info'), 404);
    return $view;
  }

  public function renderJSONError($e): BaseJSONView {
    $view = new BaseJSONView();
    $view->error('Invalid endpoint: ' . $this->param('path_info'), 404);
    return $view;
  }
}

class ApiRunner {
  protected
    $listeners,
    $map,
    $pathInfo,
    $params,
    $paramNames;

  public function __construct($map) {
    $this->map = $map;
    $this->paramNames = [];
    $this->params = [];
    $this->listeners = [];
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
      $_GET = array_merge($_GET, $this->params);
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
    foreach ($this->map as $v) {
      if ($this->matches($this->getPathInfo(), $v['route'])) {
        return $v['controller'];
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

  public function addEventListener(string $event, string $controller): void {
    if (class_exists($controller)) {
      $this->listeners[$event][] = $controller;
    } else {
      throw new Exception('Listener not found: ' . $controller);
    }
  }

  public function fireEvent($event): void {
    if (!idx($this->listeners, $event)) {
      return;
    }

    foreach ($this->listeners[$event] as $controller_name) {
      $controller = new $controller_name(
        $this->getPathInfo(),
        $this->getParams(),
        $this->getFiles(),
        $this->canAccessRestrictedEndpoints());
    }
  }

  public function run() {

    $this->fireEvent('preprocess');

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

    $controller_path = 'controllers/' . $controller_path . '.hh';

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

    $this->fireEvent('controllerEnd');
    return $controller;
  }
}

class BaseRouter {
  static protected array $params = [];

  static private function getRouteByName(string $routeName): ?string {
    global $map;
    if (isset($map[$routeName])) {
      return $map[$routeName]['route'];
    }
    return null;
  }

  static public function getParameterizedRoute(
    string $route, mixed $matches, $params = []
  ): URL {
    foreach ($matches[0] as $m) {
      $isMandatoryParam = $m[0] === '/' ? true : false;
      $paramName = $isMandatoryParam
        ? str_replace('/:', '', $m)
        : preg_replace('/\(\/:(\w+)\)/i', '$1', $m);
      $param = idx($params, $paramName);
      self::$params[$paramName] = $param;
      $find = $isMandatoryParam && $param
        ? ':' . $paramName
        : '(/:' . $paramName . ')';
      $replace = !$isMandatoryParam && $param
        ? '/' . $param : $param;

      if ($isMandatoryParam && !$param) {
        invariant_violation($paramName . ' is a mandatory parameter.');
      }
      $route = str_replace($find, $replace, $route);
    }
    return new URL($route);
  }

  static public function addOptionalParameter(
    URL $url, mixed $optionalParams
  ): URL {
    $paramsKeys = array_keys(self::$params);
    $optionalParamsKeys = array_keys($optionalParams);
    $diffArray = array_diff($optionalParamsKeys, $paramsKeys);
    foreach ($diffArray as $p) {
      $url->query($p, $optionalParams[$p]);
    }
    return $url;
  }

  static public function generateUrl(string $routeName,
    $params = []
  ): URL {
    $route = self::getRouteByName($routeName);
    preg_match_all('#\(?\/:\w+\)?#', $route, $matches);
    $url = self::getParameterizedRoute($route, $matches, $params);
    if ($params) {
      $url = self::addOptionalParameter($url, $params);
    }
    return $url;
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
        'Controller' => 'controllers',
        'Worker' => 'workers',
        'Listener' => 'listeners',
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
        case 'BaseWorkerScheduler':
        case 'BaseWorker':
        case 'BaseListener':
        return;
      }

      $pattern = '/^(\w+)(' . implode('|', array_keys($map)) . ')$/';

      $class_type = [];
      if (preg_match($pattern, $class, $class_type)) {
        $dir = array_pop($class_type);
        require $map[$dir] . DIRECTORY_SEPARATOR . str_replace($dir, '', $class) . '.hh';
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

class MongoInstance {
  protected static $db = [];
  public static function get($collection = null, $with_collection = false) {
    if (strpos($collection, 'mongodb://') !== false) {
      $db_url = explode('/', $collection);
      $collection = $with_collection ? array_pop($db_url) : null;
      $db_url = implode('/', $db_url);
      $parts = parse_url($db_url);
      if (idx($parts, 'user') && idx($parts, 'pass')) {
        $auth_pattern = sprintf('%s:%s@', $parts['user'], $parts['pass']);
        $db_url = str_replace($auth_pattern, '', $db_url);
      }

    } elseif (isset($_ENV['MONGOHQ_URL'])) {
      $db_url = $_ENV['MONGOHQ_URL'];
      $parts = parse_url($db_url);
      if (idx($parts, 'user') && idx($parts, 'pass')) {
        $auth_pattern = sprintf('%s:%s@', $parts['user'], $parts['pass']);
        $db_url = str_replace($auth_pattern, '', $db_url);
      }
    } else {
      l('MongoInstance: No MONGOHQ_URL specified or invalid collection.');
      l(sprintf('MONGOHQ_URL: %s, collection: %s',
        idx($_SERVER, 'MONGOHQ_URL'),
        $collection));
      l('_ENV[MONGOHQ_URL]:', $_ENV['MONGOHQ_URL']);

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

  final public function section(string $section): :x:frag {
    invariant(
      isset($this->sections),
      'At least one section need to be defined');

    invariant(
      idx($this->sections, $section) !== null,
      'Section %s does not exist', $section);

    return $this->sections[$section];
  }

  final public function hasSection(string $section): bool {
    return !!idx($this->sections, $section, false);
  }

  abstract public function render(): :x:element;
}

class :base:widget extends :x:element {
  public final function css(mixed $url): void {
    invariant(is_string($url) || is_array($url), 'url must be array or string');

    if (is_array($url)) {
      foreach ($url as $u) {
        invariant(is_string($u), 'Invalid string provided');
        BaseLayoutHelper::addStylesheet(<link rel="stylesheet" href={$u} />);
      }
    } elseif (is_string($url)) {
      BaseLayoutHelper::addStylesheet(<link rel="stylesheet" href={$url} />);
    }
  }

  public final function js(mixed $url): void {
    invariant(is_string($url) || is_array($url), 'url must be array or string');

    if (is_array($url)) {
      foreach ($url as $u) {
        invariant(is_string($u), 'Invalid string provided');
        BaseLayoutHelper::addJavascript(<script src={$u}></script>);
      }
    } elseif (is_string($url)) {
      BaseLayoutHelper::addJavascript(<script src={$url}></script>);
    }
  }
}

class BaseLayoutHelper {
  protected static array $javascripts;
  protected static array $stylesheets;

  public static function addJavascript(:script $javascript): void {
    $url = $javascript->getAttribute('src');

    if (!idx(self::$javascripts, $url)) {
      self::$javascripts[$url] = $javascript;
    }
  }

  public static function addStylesheet(:link $stylesheet): void {
    $url = $stylesheet->getAttribute('href');

    if (!idx(self::$stylesheets, $url)) {
      self::$stylesheets[$url] = $stylesheet;
    }
  }

  public static function javascripts(): array<:script> {
    return self::$javascripts === null ? [] : self::$javascripts;
  }

  public static function stylesheets(): array<:link> {
    return self::$stylesheets === null ? [] : self::$stylesheets;
  }
}

class BaseTranslationHolder {
  static array $projects;

  static protected function loadProject(string $locale, string $project): bool {
    if (static::$projects == null) {
      static::$projects[$locale] = [];
    }

    if (idx(static::$projects[$locale], $project)) {
      return true;
    }

    $project_path = sprintf('projects/%s/%s.json', $locale, $project);

    if (!file_exists($project_path)) {
      l('The requested translation project is missing:', $project_path);
      static::$projects[$locale][$project] = [];
      return false;
    } else {
      static::$projects[$locale][$project] =
        json_decode(file_get_contents($project_path));

      invariant(
        json_last_error() == JSON_ERROR_NONE,
        'Failed parsing translation project: ', $project_path);

      return true;
    }
  }

  static public function translation(
    string $locale,
    string $project,
    string $key): string {
    if (!isset(static::$projects[$locale][$project])) {
      static::loadProject($locale, $project);
    }
    $key = trim($key);
    return idx(static::$projects[$locale][$project], $key, $key);
  }
}

class :t extends :x:primitive {
  category %phrase, %flow;
  children (pcdata | %translation)*;
  attribute
    string locale,
    string project,
    string description;

  public function init() {
    if ($this->getAttribute('locale')) {
      $this->locale = $this->getAttribute('locale');
    } elseif (EnvProvider::getLocale()) {
      $this->locale = EnvProvider::getLocale();
    } else {
      invariant_violation('No default locale provided');
    }
  }

  public function stringify() {
    $key = '';
    $tokens = [];
    foreach ($this->getChildren() as $elem) {
      if (is_string($elem)) {
        $key .= $elem;
      } else {
        $token_key = sprintf('{%s}', $elem->getAttribute('name'));
        $tokens[$token_key] = $elem->stringify();
        $key .= $token_key;
      }
    }

    $translation = BaseTranslationHolder::translation(
      $this->locale,
      $this->getAttribute('project'),
      $key);

    foreach ($tokens as $token_key => &$token) {
      $translated_token = BaseTranslationHolder::translation(
        $this->locale,
        $this->getAttribute('project'),
        $token_key);

      $token = $translated_token === $token_key ?
        $token :
        $translated_token;
    }

    return str_replace(
      array_keys($tokens),
      array_values($tokens),
      $translation);
  }
}

class :t:p extends :x:primitive {
  category %translation;
  children (pcdata);
  attribute
    string name @required;

  public function stringify() {
    return idx($this->getChildren(), 0);
  }
}
