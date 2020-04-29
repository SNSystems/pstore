# pstore-index-stats

This tool is used to dump (as [comma-separated-values](https://en.wikipedia.org/wiki/Comma-separated_values)) a small collection of statistics which can be useful to understand and evaluate the state of the various indices in a pstore file.

Example output:

    name,branching-factor,mean-leaf-depth,max-depth,size
    compilation,6.25263,3.124,5,500
    fragment,4.1554,5.79871,10,25050000
    name,4.16371,5.79568,8,25050501

For each index it computes the following:

| Statistic       | Description |
| --------------- + ----------- |
| Branching Factor | The [branching factor](https://en.wikipedia.org/wiki/Branching_factor) is the average number of children at each internal node (the node’s out-degree). Larger numbers for this statistic tend to indicate that, on average, fewer internal nodes must be visited when finding a specific key. |
| Mean Leaf Depth | The mean number of nodes that must be visited when locating a specific key. Smaller numbers indicate a shorter average search path.
| Max Depth       | The maximum number of nodes that must be visited when locating a specific key. Together with the mean depth, this conveys an impression of how well balanced the tree is. This value should not be too much greater than the mean leaf depth.
| Size            | The number of leaves in the index. |
