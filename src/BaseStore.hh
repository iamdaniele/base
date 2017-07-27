<?hh
abstract class BaseStore {

  protected $class;
  protected $db;

  protected static $instance;

  public function __construct(
    ?string $collection = null,
    ?string $class = null) {
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

  protected static function i(): this {
    return new static();
  }

  public function docs() {
    return $this->docs;
  }

  public function find(?array $query = [], ?array $fields = []) {
    $i = static::i();
    $i->docs = $i->db->find($query, $fields);
    return $i;
  }

  public function findOne(?array $query = [], ?array $fields = []) {
    $i = static::i();
    $class = $i->class;
    $doc = $i->db->findOne($query, $fields);
    return $i->loadModel($doc);
  }

  public function update(
    array $query,
    array $new_object,
    ?array $options = []
  ) {
    $i = static::i();
    return $i->db->update($query, $new_object, $options);
  }

  public function sort(array $query)  {
    $this->docs = $this->docs->sort($query);
    return $this;
  }

  public function skip(int $skip) {
    $this->docs = $this->docs->skip($skip);
    return $this;
  }

  public function limit(int $query) {
    $this->docs = $this->docs->limit($query);
    return $this;
  }

  public function load() {
    $class = $this->class;
    foreach ($this->docs as $doc) {
      yield new $class($doc);
    }
  }

  public function loadModel(?array $doc = []) {
    if (!$doc) {
      return null;
    }
    $class = $this->class;
    return new $class($doc);
  }

  public function distinct(string $key, array $query = []) {
    $docs = static::i()->db->distinct($key, $query);
    return !is_array($docs) ? [] : $docs;
  }

  public function findById(MongoId $id) {
    return static::i()->findOne(['_id' => $id]);
  }

  public function count(array $query = []): int {
    return static::i()->db->count($query);
  }

  protected function ensureType(BaseModel $item): bool {
    return class_exists(static::i()->class) && is_a($item, $this->class);
  }

  public function removeByModel(BaseModel $item) {
    if (!static::i()->ensureType($item)) {
      throw new Exception(
        'Invalid object provided, expected ' . static::i()->class);
      return false;
    }

    if ($item == null) {
      return false;
    }

    try {
      if ($item->_id === null) {
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

  public function remove(array $query = [], array $options = []) {
    try {
      static::i()->db->remove($query, $options);
      return true;
    } catch (MongoException $e) {
      l('MongoException:', $e->getMessage());
      return false;
    }
  }

  public function removeById(MongoId $id) {
    return static::i()->remove(['_id' => $id]);
  }

  public function aggregate(BaseAggregation $aggregation) {
    return call_user_func_array(
      [static::i()->db, 'aggregate'],
      $aggregation->getPipeline());
  }

  public function mapReduce(
    MongoCode $map,
    MongoCode $reduce,
    ?array $query = null,
    ?array $config = null) {

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
      if ($item->_id === null) {
        $id = new MongoId();
        $item->_id = $id;
        $document = $item->document();
        static::i()->db->insert($document);
      } else {
        $document = $item->document();
        static::i()->db->save($document);
      }
      return true;
    } catch (MongoException $e) {
      invariant_violation($e->getMessage());
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
  private string $__model;
  public function __construct(array<string, mixed> $document = []) {
    $this->__model = get_called_class();
    foreach ($document as $key => $value) {
      if (property_exists($this, $key)) {
        $this->$key = $key == '_id' ? mid($value) : $value;

        if (is_array($value)) {
          if (idx($value, '__model') && !idx($value, '__ref')) {
            $model_name = idx($value, '__model');
            $model = new $model_name($value);
            $this->$key = $model;
          } elseif (idx($value, '__ref')) {
            $model_name = idx($value, '__model');
            $model = new $model_name();
            $model->_id = idx($value, '_id');
            $this->$key = BaseRef::fromModel($model);
          } else {
            $refs = [];
            foreach ($value as $k => $v) {
              if (idx($v, '__model') && !idx($v, '__ref')) {
                $model_name = idx($v, '__model');
                $model = new $model_name($v);
                $refs[$k] = $model;
              } elseif (idx($v, '__ref')) {
                $model_name = idx($v, '__model');
                $model = new $model_name();
                $model->_id = idx($v, '_id');
                $refs[$k] = BaseRef::fromModel($model);
              } else {
                $refs[$k] = $v;
              }
            }

            $this->$key = $refs;
          }
        }
      }
    }
  }

  public function __set($name, $value) {
    if (get_called_class() !== 'BaseModel') {
      invariant_violation(
        'Cannot set field %s in %s: field does not exist',
        $name,
        get_called_class());
    }
  }

  public function __get($name) {
    if (get_called_class() !== 'BaseModel') {
      invariant_violation(
        'Cannot get field %s in %s: field does not exist',
        $name,
        get_called_class());
    }
  }

  public function document(): array<string, mixed> {
    $document = get_object_vars($this);
    foreach ($document as &$item) {
      if ($item instanceof BaseRef || $item instanceof BaseModel) {
        $item = $item->document();
      } elseif (is_array($item)) {
        foreach ($item as &$i) {
          $i = $i instanceof BaseRef || $i instanceof BaseModel ?
            $i->document() :
            $i;
        }
      }
    }

    return $document;
  }

  final public function reference(): BaseRef {
    return BaseRef::fromModel($this);
  }
}

class BaseRef<T as BaseModel> {
  protected bool $__ref;
  protected string $__model;
  protected string $__collection;
  protected MongoId $_id;
  protected ?T $model;

  public function __construct(T $model) {
    invariant(
      $model->_id,
      'Cannot create a reference from a non-existing document');

    invariant(
      $model::COLLECTION,
      'Cannot create a reference from Models with no collections');

    $this->__ref = true;
    $this->_id = $model->_id;
    $this->__model = get_class($model);
    $this->__collection = $model::COLLECTION;
  }

  public function __set($key, $value) {
    invariant_violation('Cannot set attributes on a reference');
  }

  public function __get($key): mixed {
    // no idx() here, will trust BaseModel's __get()
    return $this->model() !== null ? $this->model->$key : null;
  }

  public static function fromModel(T $model): BaseRef<T> {
    return new self<T>($model);
  }

  public function model(): ?T {
    if ($this->model) {
      return $this->model;
    }

    $store = MongoInstance::get($this->__collection);
    $doc = $store->findOne(['_id' => $this->_id]);
    if ($doc === null) {
      ls('Broken reference: %s:%s', $this->__collection, $this->_id);
      return null;
    }

    $model = $this->__model;
    $this->model = new $model($doc);
    return $this->model;
  }

  public function document(): array<string, mixed> {
    return [
      '__ref' => true,
      '_id' => $this->_id,
      '__model' => $this->__model,
      '__collection' => $this->__collection,
    ];
  }
}
