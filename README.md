LEVEL: C-core

START: signle core start command: loadboot
       double cores start command: loadbootm

shell command:               usage:                   parameter_num:        description:
      1) ps                  ps                       0                     show pcb
      2) clear               clear                    0                     clear the screen
      3) exec                exec "task_id"           1                     spawn a process/task
      4) kill                kill "pid"               1                     kill a process
      5) exit                exit                     0                     kill current process
      6) waitpid             waitpid "pid"            1                     wait until targeted process exit
      7) taskset          a) taskset "mask" "task_id" 2                     spwan a process/task on certain core
                          b) taskset -p "mask" "pid"  2(3)                  change targeted process's mask