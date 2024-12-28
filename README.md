### Tracker协议

#### 请求参数

- tracker url
- info_hash 需要 urlencode
- peer_id 需要urlencode
- ip optional
- port
- uploaded
- downloaded
- left
- event: start completed stopped

#### 响应

正常返回的相应是bencode是个字典，重要的是里面的peers建，有两种模式，第一种是紧凑模式
第二种是列表， 列表里每一个项是字典

##### 紧凑模式

6\*n的byte流,n 是peer个数， 前面四个字节是 ip，后面两个字节是port

##### 非紧凑模式

列表里嵌套字典，每个字典代表一个peer，
一个字典有三个键，分别是：

1. peer id 字符串
2. ip 字符串
3. port 数字
