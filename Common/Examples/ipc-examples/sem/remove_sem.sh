#!/bin/csh -f
set l = `ipcs -s | grep "$USER"| cut -c12-19`
foreach s ( $l )
    echo $s
    ipcrm sem $s 
end
if ($#l != 0 ) echo $#l semaphore\(s\) for $user removed
