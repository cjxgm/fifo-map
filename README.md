# fifo-map

Copyright (C) Giumo Clanjor (哆啦比猫/兰威举), 2019-2020.
Licensed under the MIT License.

A hash map that guarantees iteration in insertion-order for C++14 or above.
Or you can say, "a FIFO-ordered associative container" if you feel like it.

(Yes, it's similar to `nlohmann::fifo_map`, but this one uses
`std::unordered_map` instead of `std::map`.)

It's basically an `std::forward_list` of key/value pairs,
with a lookup index built using `std::unordered_map`.

It's a drop-in replacement for `std::unordered_map`
if the interface you used was implemented.

Time complexity of all implemented operations are basically
the same as `std::unordered_map`.

