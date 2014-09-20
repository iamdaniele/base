<?hh
class BaseWorkerScheduler {
  const string SCHEDULER_KEY = 'workers';
  protected static $queue;
  protected static function initQueue(): void {
    invariant(
      idx($_ENV, 'REDISCLOUD_URL'),
      'Please specify an instance of Redis');

    self::$queue = new Predis\Client([
      'host' => parse_url($_ENV['REDISCLOUD_URL'], PHP_URL_HOST),
      'port' => parse_url($_ENV['REDISCLOUD_URL'], PHP_URL_PORT),
      'password' => parse_url($_ENV['REDISCLOUD_URL'], PHP_URL_PASS)]);
  }

  public static function run(BaseWorker $worker): void {
    if (!self::$queue) {
      self::initQueue();
    }

    try {
      $worker->beforeRun();
    } catch (Exception $e) {
      invariant_violation(
        '%s: %s failed precheck: %s',
        __CLASS__,
        get_class($worker),
        $e->getMessage());
    }

    self::$queue->rpush(self::SCHEDULER_KEY, serialize($worker));
  }
}

abstract class BaseWorker {
  public final function __construct() {
    $this->init();
  }

  public function shouldRetry(): bool {
    return false;
  }

  public function beforeRun(): void {}

  public function init(): void {}
  abstract public function run(): void;
}
