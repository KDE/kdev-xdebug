<?php
function foo() {
    static $i=0;
    $i++;
    echo $i."\n";
}
foo();
foo();
