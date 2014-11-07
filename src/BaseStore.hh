<?hh
abstract class BaseStore {
  protected $class;
  protected $db;

  protected static $instance;

  public function __construct(string $collection = null, string $class = null) {
    if (defined('static::COLLECTION') && defined('static::MODEL')) {
      $collection = static::COLLECTION;
      $class = static::MODEL;
    }

    invariant($collection && $class, 'Collection or class not provided');

    $this->collection = $collection;
    $this->class = $class;
    $this->db = MongoInstance::get($collection);
    static::$instance = $this;
  }

  protected static function i() {
    return new static();
  }

  public function db() {
    return static::i()->db;
  }

  public function find(
    array $query = [],
    ?int $skip = 0,
    ?int $limit = 0): BaseModel {
    $docs = static::i()->db->find($query);
    $class = static::i()->class;

    if ($skip !== null) {
      $docs = $docs->skip($skip);
    }

    if ($limit !== null) {
      $limit = $docs->limit($limit);
    }

    foreach ($docs as $doc) {
      yield new $class($doc);
    }
  }

  public function distinct(string $key, array $query = []) {
    $docs = static::i()->db->distinct($key, $query);
    return !is_array($docs) ? [] : $docs;
  }

  public function findOne(array $query): ?BaseModel {
    $doc = static::i()->db->findOne($query);
    $class = static::i()->class;
    return $doc ? new $class($doc) : null;
  }

  public function findById(MongoId $id): ?BaseModel {
    return static::i()->findOne(['_id' => $id]);
  }

  public function count(array $query): int {
    return static::i()->db->count($query);
  }

  protected function ensureType(BaseModel $item): bool {
    return class_exists(static::i()->class) && is_a($item, $this->class);
  }

  public function remove(BaseModel $item) {
    if (!static::i()->ensureType($item)) {
      throw new Exception(
        'Invalid object provided, expected ' . static::i()->class);
      return false;
    }

    if ($item == null) {
      return false;
    }

    try {
      if ($item->getID()) {
        static::i()->db->remove($item->document());
        return true;
      } else {
        return false;
      }
    } catch (MongoException $e) {
      l('MongoException:', $e->getMessage());
      return false;
    }
  }

  public function removeWhere($query = []) {
    try {
      static::i()->db->remove($query);
      return true;
    } catch (MongoException $e) {
      l('MongoException:', $e->getMessage());
      return false;
    }
  }

  public function removeById($id) {
    return static::i()->removeWhere(['_id' => mid($id)]);
  }

  public function aggregate(BaseAggregation $aggregation) {
    return call_user_func_array(
      [static::i()->db, 'aggregate'],
      $aggregation->getPipeline());
  }

  public function mapReduce(
    MongoCode $map,
    MongoCode $reduce,
    array $query = null,
    array $config = null) {

    $options = [
      'mapreduce' => static::i()->collection,
      'map' => $map,
      'reduce' => $reduce,
      'out' => ['inline' => true]];

    if ($query) {
      $options['query'] = $query;
    }

    if ($config) {
      unset($options['mapreduce']);
      unset($options['map']);
      unset($options['reduce']);
      unset($options['query']);
      $options = array_merge($options, $config);
    }

    $res = MongoInstance::get()->command($options);

    if (idx($res, 'ok')) {
      return $res;
    } else {
      l('MapReduce error:', $res);
      return null;
    }
  }

  public function save(BaseModel &$item) {
    if (!static::i()->ensureType($item)) {
      throw new Exception(
        'Invalid object provided, expected ' . static::i()->class);
      return false;
    }

    if ($item == null) {
      return false;
    }

    try {
      if (!$item->getID()) {
        $id = new MongoId();
        $item->setID($id);
        $document = $item->document();
        static::i()->db->insert($document);
      } else {
        $document = $item->document();
        static::i()->db->save($document);
      }
      return true;
    } catch (MongoException $e) {
      l('MongoException:', $e->getMessage());
      return false;
    }
    return true;
  }
}

class BaseStoreCursor {
  protected
    $class,
    $count,
    $cursor,
    $next;

  public function __construct($class, $count, $cursor, $skip, $limit) {
    $this->class = $class;
    $this->count = $count;
    $this->cursor = $cursor;
    $this->next = $count > $limit ? $skip + 1 : null;
  }

  public function count() {
    return $this->count;
  }

  public function nextPage() {
    return $this->next;
  }

  public function docs() {
    $class = $this->class;
    foreach ($this->cursor as $entry) {
      yield new $class($entry);
    }
  }
}

class BaseAggregation {
  protected $pipeline;
  public function __construct() {
    $this->pipeline = [];
  }

  public function getPipeline() {
    return $this->pipeline;
  }

  public function project(array $spec) {
    if (!empty($spec)) {
      $this->pipeline[] = ['$project' => $spec];
    }
    return $this;
  }

  public function match(array $spec) {
    if (!empty($spec)) {
      $this->pipeline[] = ['$match' => $spec];
    }
    return $this;
  }

  public function limit($limit) {
    $this->pipeline[] = ['$limit' => $limit];
    return $this;
  }

  public function skip($skip) {
    $this->pipeline[] = ['$skip' => $skip];
    return $this;
  }

  public function unwind($field) {
    $this->pipeline[] = ['$unwind' => '$' . $field];
    return $this;
  }

  public function group(array $spec) {
    if (!empty($spec)) {
      $this->pipeline[] = ['$group' => $spec];
    }
    return $this;
  }

  public function sort(array $spec) {
    if (!empty($spec)) {
      $this->pipeline[] = ['$sort' => $spec];
    }
    return $this;
  }

  public static function addToSet(string $field): array {
    return ['$addToSet' => '$' . $field];
  }

  public static function sum($value = 1): array {
    if (!is_numeric($value)) {
      $value = '$' . $value;
    }
    return ['$sum' => $value];
  }

  public function __call($name, $args) {
    $field = array_pop($args);
    switch ($name) {
      case 'addToSet':
      case 'first':
      case 'last':
      case 'max':
      case 'min':
      case 'avg':
        return ['$' . $name => '$' . $field];
    }

    throw new RuntimeException('Method not found: ' . $name);
  }

  public function push($field) {
    if (is_array($field)) {
      foreach ($field as &$f) {
        $f = '$' . $f;
      }
    } else {
      $field = '$' . $field;
    }

    return ['$push' => $field];
  }
}

abstract class BaseModel {
  public ?MongoId $_id;
  public function __construct(array<string, mixed> $document = []) {

    foreach ($document as $key => $value) {
      if (property_exists($this, $key)) {
        $this->$key = $key == '_id' ? mid($value) : $value;
      }
    }
  }

  public function document(): array<string, mixed> {
    return get_object_vars($this);
  }

  final public function getID(): ?MongoId {
    return $this->_id;
  }

  final public function setID(MongoId $_id): void {
    $this->_id = $_id;
  }

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

    // $method = preg_replace('/^(get|set|remove|has)/i', '', $method);
    $method = preg_replace('/^(get|set)/i', '', $method);

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

    $arg = array_pop($args);

    switch ($op) {
      case 'set':
        $this->$field = $arg;
        return;

      case 'get':
        invariant(
          property_exists($this, $field),
          '%s is not a valid field for %s',
          $field,
          get_called_class());
        return $this->$field;
    }
  }
}
