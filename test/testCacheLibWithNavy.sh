cd ../build

# 
./CacheTest -write=true -read=true -read_inverse=false -operation_num=100000 > ../log/testCacheLib.log
