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

  public static function schedule(BaseWorker $worker): void {
    if (!self::$queue) {
      self::initQueue();
    }

    self::$queue->rpush(self::SCHEDULER_KEY, serialize($worker));
  }
}

abstract class BaseWorker {
  public final function __construct() {
    $this->init();
  }

  public function init(): void {}
  abstract public function run(): void;
}
