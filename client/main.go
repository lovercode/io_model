package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"net"
	"sync"
	"sync/atomic"
	"time"
)

var (
	server = flag.String("s", "127.0.0.1:8888", "服务端地址")
	start  = flag.Int("start", 10, "起始并发数")
	end    = flag.Int("end", 10, "结束并发数")
	step   = flag.Int("step", 10, "步进")
	ts     = flag.Float64("t", 10, "持续时间秒")
	b      = flag.Int("b", 1024, "单次数据大小")
)

type res struct {
	client  int
	counter int64
	qps     int64
	avgCost int64
}

func benchMark(c int) *res {
	wg := &sync.WaitGroup{}
	start := time.Now()
	var counter = &atomic.Int64{}
	var times = &atomic.Int64{}
	for i := 0; i < c; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			conn, err := net.Dial("tcp", *server)
			if err != nil {
				return
			}
			for {
				if time.Now().Sub(start).Seconds() >= *ts {
					conn.Close()
					break
				}
				counter.Add(1)
				start := time.Now()
				data := make([]byte, *b+4)
				// 使用binary包将int32值编码为字节序列
				// binary.BigEndian指定大端模式，也可以选择binary.LittleEndian
				binary.BigEndian.PutUint32(data, uint32(*b))
				// 将字节序列写入连接
				_, err := conn.Write(data)
				if err != nil {
					fmt.Println(err)
				}
				lengthBytes := make([]byte, 4)
				_, err = conn.Read(lengthBytes)
				if err != nil {
					fmt.Println("Error reading:", err.Error())
					break
				}
				length := binary.BigEndian.Uint32(lengthBytes)

				// 读取数据
				data = make([]byte, length)
				_, err = conn.Read(data)
				if err != nil {
					fmt.Println("Error reading:", err.Error())
					break
				}
				times.Add(time.Now().Sub(start).Microseconds())
			}
		}()
	}
	wg.Wait()

	return &res{
		counter: counter.Load(),
		qps:     counter.Load() / int64(*ts),
		avgCost: times.Load() / counter.Load(),
		client:  c,
	}

}

func main() {
	flag.Parse()
	f := "\t\t"
	fmt.Printf("并发数%v请求数%vqps%vus%vms\n", f, f, f, f)
	for i := *start; i <= (*end); i += *step {
		res := benchMark(i)
		fmt.Printf("%v%v%v%v%v%v%v%v%v\n",
			res.client, f, res.counter, f, res.qps, f, res.avgCost, f, res.avgCost/1000)
	}
}
