# CacheStat统计

Trace page cache hit/miss ratio，基于4.18内核代码。

## 内核函数

1.  add_to_page_cache_lru
2.  mark_page_accessed
3.  folio_account_dirtied
4.  account_page_dirtied
5.  mark_buffer_dirty

## 统计公式

```
total number of accesses = number of mark_page_accessed() - number of mark_buffer_dirty()
total number of misses = number of add_to_page_cache_lru() - number of account_page_dirtied()
```

