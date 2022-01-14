For Part 1, nothing special to note.

For Part 2, C-core is finished. Some settings are explained as follows:
1) Notice that in the test for fork, the father process can be only forked once. Hence only one child process will be created each time.
2) The priority variable only have 5 values: 0~4, other values will not be accepted.
3) Because the maximum number of PCB is 16, DO NOT fork more than 5 times. (change NUM_MAX_PCB otherwise)