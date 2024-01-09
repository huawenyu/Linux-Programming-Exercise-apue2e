# bsearch:

> Description.
> [More information](https://url-to-upstream.tld).


# bsearch sample:

```c
/**
>>> {fileout}
>>> {fileout}  3
>>> {fileout}  4
>>> {fileout}  18
*/

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <limits.h>

    struct mi {
        int nr;
        char *name;
    } months[] = {
        { 1, "Jan" }, { 2, "Feb" }, { 3, "Mar" }, { 4, "Apr" }, { 4, "Apr_" }, { 4, "Apr__" },
        { 5, "May" }, { 6, "Jun" }, { 7, "Jul" }, { 8, "Aug" },
        { 9, "Sep" }, {10, "Oct" }, {11, "Nov" }, {12, "Dec" }, {INT_MAX, "_End_"}
    };

    #define nr_of_months (sizeof(months)/sizeof(months[0]))

    static int compmi(const void *key, const void *elt)
    {
        struct mi *k = (struct mi *) key;
        struct mi *e = (struct mi *) elt;

        if (k->nr < e->nr) return -1;
        else if (k->nr > e->nr) return 1;
        return 0;
    }

    int main(int argc, char **argv)
    {
        int i;

        qsort(months, nr_of_months, sizeof(struct mi), compmi);

        if (argc == 1) {
            printf("List:\n");
            for (i = 0; i < sizeof(months)/sizeof(months[0]); i++) {
                struct mi *res = &months[i];
                printf("    %s is the #%d month!\n", res->name, res->nr);
            }
        }

        printf("Find:\n");
        for (i = 1; i < argc; i++) {
            struct mi key, *res;
            key.nr = atoi(argv[i]);

            res = bsearch(&key, months, nr_of_months,
                    sizeof(struct mi), compmi);
            if (res == NULL)
                printf("    unknown month key.nr=%d\n", key.nr);
            else
                printf("    %s is the #%d month!\n", res->name, res->nr);
        }

        exit(EXIT_SUCCESS);
    }
```

# Search insert position of K in a sorted array

Given a sorted array arr[] consisting of N distinct integers and an integer K,
- the task is to find the index of K, if it’s present in the array arr[].
- Otherwise, find the index where K must be inserted to keep the array sorted.

```c
//
// find the lower insertion point of an element in a sorted array
//
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <limits.h>

    // Function to return the lower insertion point
    // of an element in a sorted array
    int LowerInsertionPoint(int arr[], int n, int X)
    {

        // Base cases
        if (X < arr[0])
            return 0;
        else if (X > arr[n - 1])
            return n;

        int lowerPnt = 0;
        int i = 1;

        while (i < n && arr[i] < X) {
            lowerPnt = i;
            i = i * 2;
        }

        // Final check for the remaining elements which are < X
        // Lower: arr[lowerPnt] < X
        // Higher: arr[lowerPnt] <= X
        while (lowerPnt < n && arr[lowerPnt] <= X)
            lowerPnt++;

        return lowerPnt;
    }

    // Driver code
    int main()
    {
        int arr[] = { 2, 3, 4, 4, 5, 6, 7, 9 };
        int n = sizeof(arr) / sizeof(arr[0]);

        int X = 4;
        int res = LowerInsertionPoint(arr, n, X);
        printf("    '%d' is insert-index=%d\n", X, res);
        return 0;
    }

```

## iterate one by one

```c
/**
Time Complexity: O(N)
Auxiliary Space: O(1)

*/

    #include <stdio.h>

    // Function to find insert position of K
    int find_index(int arr[], int n, int K)
    {
        // Traverse the array
        for (int i = 0; i < n; i++)
            // If K is found
            if (arr[i] == K)
                return i;
            // If current array element exceeds K
            else if (arr[i] > K)
                return i;
        // If all elements are smaller than K
        return n;
    }

    // Driver Code
    int main()
    {
        int arr[] = { 1, 3, 5, 6 };
        int n = sizeof(arr) / sizeof(arr[0]);
        int K = 2;
        printf("%d\n", find_index(arr, n, K));
        return 0;
    }
```

## improve by binary search

```c
/**
Time Complexity: O(log N)
Auxiliary Space: O(1)

*/

    #include<stdio.h>

    // Function to find insert position of K
    int find_index(int arr[], int n, int K)
    {
        // Lower and upper bounds
        int start = 0;
        int end = n - 1;
        // Traverse the search space
        while (start <= end) {
            int mid = (start + end) / 2;
            // If K is found
            if (arr[mid] == K)
                return mid;
            else if (arr[mid] < K)
                start = mid + 1;
            else
                end = mid - 1;
        }
        // Return insert position
        return end + 1;
    }

    // Driver Code
    int main()
    {
        int arr[] = { 1, 3, 5, 6 };
        int n = sizeof(arr) / sizeof(arr[0]);
        int K = 2;
        printf("%d",find_index(arr, n, K));
        return 0;
    }
```

