<?php
require_once 'common.php';
require_once 'BaseParam.php';
require_once 'Base.php';

Base::registerAutoloader();

$app = new ApiRunner($map);
$app->run();
