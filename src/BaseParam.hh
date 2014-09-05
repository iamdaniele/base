<?hh
class BaseParam {
  protected $name;
  protected $value;
  protected $required;
  public function __construct($key, $value) {
    $this->name = $key;
    $this->value = $value;
    $this->required = true;
  }

  public function name() {return $this->name;}
  public function value() {return $this->value;}
  public function required() {$this->required = true; return $this;}
  public function isRequired() {return $this->required = true;}

  public static function IntType($key, $default = null) {
    $params = array_merge($_GET, $_POST, $_FILES);
    $value = idx($params, $key);

    invariant(!($value === null && $default === null),
      'Param is required: ' . $key);

    $value = $value !== null ? $value : $default;
    $value = filter_var($value, FILTER_SANITIZE_NUMBER_INT);
    $value = filter_var($value, FILTER_VALIDATE_INT);
    invariant($value !== false, 'Wrong type: %s', $key);
    return new BaseParam($key, $value);
  }

  public static function FloatType(
    $key,
    $default = null) {

    $params = array_merge($_GET, $_POST, $_FILES);
    $value = idx($params, $key);

    invariant(!($value === null && $default === null),
      'Param is required: ' . $key);

    $value = $value !== null ? $value : $default;
    $value = filter_var(
      $value,
      FILTER_SANITIZE_NUMBER_FLOAT,
      FILTER_FLAG_ALLOW_FRACTION);

    $value = filter_var($value, FILTER_VALIDATE_FLOAT);
    invariant($value !== false, 'Wrong type: %s', $key);
    return new BaseParam($key, $value);
  }

  public static function ArrayType(
    $key,
    $default = null) {

    $params = array_merge($_GET, $_POST, $_FILES);
    $value = idx($params, $key);

    invariant(!($value === null && $default === null),
      'Param is required: ' . $key);

    $value = $value !== null ? $value : $default;
    invariant(is_array($value) === true, 'Wrong type: %s', $key);
    return new BaseParam($key, $value);
  }

  public static function JSONType(
    $key,
    $default = null) {

    $params = array_merge($_GET, $_POST, $_FILES);
    $value = idx($params, $key);

    invariant(!($value === null && $default === null),
      'Param is required: ' . $key);

    $value = $value !== null ? json_decode($value, true) : $default;
    invariant(JSON_ERROR_NONE === json_last_error(), 'Wrong type: %s', $key);
    return new BaseParam($key, $value);
  }

  public static function StringType(
    $key,
    $default = null) {

    $params = array_merge($_GET, $_POST, $_FILES);
    $value = idx($params, $key);

    invariant(!($value === null && $default === null),
      'Param is required: ' . $key);

    $value = $value !== null ? $value : $default;
    invariant(is_string($value), 'Wrong type: %s', $key);
    return new BaseParam($key, $value);
  }

  public static function FileType(
    $key,
    $default = null) {
    invariant(!(idx($_FILES, $key, null) === null && $default === null),
      'Param is required: ' . $key);

    invariant(idx($_FILES, $key, false), 'Wrong type: %s', $key);
    return new BaseParam($key, $_FILES[$key]);
  }
}
