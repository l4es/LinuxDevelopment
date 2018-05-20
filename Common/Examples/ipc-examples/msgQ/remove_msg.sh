#!/bin/csh -f
set l = `ipcs -q | grep "$USER"| cut -c12-19`
foreach s ( $l )
    echo $s
    ipcrm msg $s 
end
if ($#l != 0 ) echo $#l shared memory\(s\) for $user removed
