

#include<cstring>
#include<vector>
#include<map>
#include<iostream>
#include<fstream>

#include "memsim.h"

using namespace std;

//FIFO模式下的构造函数
FIFO::FIFO(bool debug, int pageNum){
    this->debug = debug;
    this->pageNum = pageNum;
    this->head = NULL;
    this->Q = new Queue(pageNum);
    this->currNum = 0;
}
//fifo 的处理函数，读入一个地址和mode，然后进行存取操作，并记录
int FIFO::handle(long address,string scheme,int* readNum, int* writeNUm){
    int index = Q->exist(address);

    //如果mode是W， 那么说内存中的这一块地址已经被改写了，那么说就要设置dirty
    if(scheme.compare("W") == 0){
        Q->markDirty(address);
    }
    //如果说address的地址不存在在内存中，那么进行处理
    if(index == -1){
        //此时处于MISS状态
        if(debug == true){ 
            cout << "MISS:     page " << address << endl;
        }
        //如果当前使用的内存小于实际内存时，只需要把addrss的数据写入空闲内存即可，不需要页面替换
        if(currNum < pageNum){
            //用于保存当前地址状态的队列结构
            Q->add(address,scheme,0);
            this->currNum ++;
            (*readNum )++;
        //如果使用内存大于或者等于实际内存就是说，没有可使用内存时，就要页面替换
        }else{  //如果要替换，那么当被替换的address的内存是dirty的那么说就需要在重写回内存时设置成DIRTY
                if(Q->head->dirty == 1){
                    if(debug){
                        cout << "REPLACE:  page " << Q->head->pageId << " (DIRTY)" << endl;
                    }
                    (*writeNUm) ++ ;
                    //不写回磁盘，可以直接替换
                }else{
                    if(debug){
                        cout << "REPLACE:  page " << Q->head->pageId  << endl;
                    }
                }
            //根据替换规则，在数据状态维护队列Q中删掉第一个元素
            Q->popFirst();
            //删掉后就有了空闲空间，加入
            Q->add(address,scheme,0);

            (*readNum) ++;
            

        }
        //如果存在于内存中那么就是HIT到了
    }else{
        if(debug == true){
            cout << "HIT:      page " << address << endl;
        }
        return 1;
    }

    return index;
}

//ARB对象初始化
ARB::ARB(bool debug, int pageNum, int interval){
    this->debug = debug;
    this->pageNum = pageNum;
    this->interval = interval;
    this->Q = new Queue(pageNum);
    this->currNum = 0;
}
//ARB模式的处理方法
int ARB::handle(long address,string scheme,int* readNum, int* writeNUm){

    int index = Q->exist(address);
    if(scheme.compare("W") == 0){
        Q->markDirty(address);
    }

    //同FIFO
    if(index == -1){
        if(debug == true){ 
            cout << "MISS:     page " << address << endl;
        }

        (*readNum) ++;
        //同FIFO
        if(currNum < pageNum){
            Q->add(address,scheme,0);
            Q->setOne(address);
            this->currNum ++;
        //同FIFO
        }else{
            //首先删掉Q中最久没有使用的节点，释放空间
            Node* temp = Q->removeLeast();
            //如果是dirty，那么标志DIRTY
            if(temp->dirty == 1){
                if(debug){
                    cout << "REPLACE:  page " << temp->pageId<< " (DIRTY)" << endl;
                }
                //需要写回内存
                (*writeNUm) ++ ;
            //替换不需要写回内存，因为不是dirty的
            }else{
                if(debug){
                    cout << "REPLACE:  page " << temp->pageId << endl;
                }
            }
            //写入内存
            Q->add(address,scheme,0);
            //将该地址空间的移位寄存器首位置1,arb算法
            Q->setOne(address);
            
        }
    }else{

        //在内存中，那么HIT
        if(debug == true){
            cout << "HIT:      page " << address << endl;
        }
        //将该address空间状态高位置1
        Q->setOne(address);
    }

    return index;


}


//WSarb模式对象
WSARB::WSARB(bool debug, int pageNum,int interval,int windowSize){
    this->debug = debug;
    this->pageNum = pageNum;
    this->interval = interval;
    this->windowSize = windowSize; 
    this->currNum = 0;
    this->process = NULL;
    

}
//主函数
int main(int argc, char * argv[]){
    //根据命令行参数，初始化各种参数
    if(argc >8 || argc <6){
        cout << "input args error "<< argc <<  endl;
        return 0;
    }
    string filePath = argv[1];
    fstream file(filePath.c_str()); // 任务文件
    if(!file.is_open()){
        cout << "error " << file.is_open() << endl;
    };
    string debugModel = "debug";
    string quietModel = "quiet";
    bool debug = true ? debugModel.compare(argv[2]) == 0: false;

    int pageSize = 0;
    sscanf(argv[3],"%d",&pageSize);

    int pageNum = 0;
    sscanf(argv[4],"%d",&pageNum);

    string scheme = argv[5];

    int interval = 0;
    int windowSize = 0;

    if(argc == 7){
        sscanf(argv[6],"%d",&interval);
    }else if(argc == 8){
        sscanf(argv[6],"%d",&interval);
        sscanf(argv[7],"%d",&windowSize);
    }

    windowSize ++;

    int totalEvent = 0;
    int readNum = 0;
    int writeNum = 0;
    int pageFaults = 0;
    int prefetchFaults = 0;

    //如果是fifo模式
    if(strcmp(scheme.c_str(),"fifo") == 0){
       
        FIFO fifo(debug,pageNum);
        string line;
        string type = "0";
        long address = 0 ;
        //获取没一行数据
        while(getline(file,line)){
            //解析没一行
            int flag = parse(line,type,&address,pageSize);
            //如果是“#”开头，就是说要切换进程，那么跳过
            if(flag == 1){
                continue;
            }
            //处理请求
            totalEvent ++;
            int f = fifo.handle(address,type,&readNum,&writeNum);
            //如果内存中不存在当前页面，就是说要页面替换，那么记录
            if(f == -1){
                pageFaults ++;
            }

        }
        //如果是arb模式
    }else if(strcmp(scheme.c_str(),"arb") == 0){

        ARB arb(debug,pageNum,interval);
        string line;
        string type = "0";
        long address = 0 ;
        int timer = 0;
        //每次读取一行数据，进行处理
        while(getline(file,line)){
            //解析数据
            int flag = parse(line,type,&address,pageSize);

            if(flag == 1){
                continue;
            }

            timer ++;
            totalEvent ++;
            int f = arb.handle(address,type,&readNum,&writeNum);

            if(f == -1){
                pageFaults ++;
            }
            //如果到达interval那么进行一次集中的刷新，就是说，此时把所有的内存移位寄存器左移一位
            if(timer == interval){
                arb.Q->update();
                timer = 0;
            }

        }
        //如果是wsarb模式
    }else if(strcmp(scheme.c_str(),"wsarb") == 0){
        WSARB wsarb(debug,pageNum,interval,windowSize);
        string line;
        string type = "0";
        long address = 0 ;
        int timer = 0;
        //同ARB和FIFO
        while(getline(file,line)){
            
            int flag = parse(line,type,&address,pageSize);
            if(flag == 1){
                //如果是# 上下文切换，进程切换
                wsarb.switchContext(address, &readNum, &writeNum, &pageFaults,&prefetchFaults);
                continue;
            }
            //到达interval，集中更新一次内存状态
            if(timer == interval){
                //wsarb.currSET->Q->update();
                wsarb.update();
                timer = 0;
            }
            //处理
            totalEvent ++;
            int f = wsarb.handle(address,type,&readNum,&writeNum);
            //同上
            if(f == -1){
                pageFaults ++;
            }
            
            timer ++;

        }
    




    }


    //统计输出

    cout << "events in trace:    "<<totalEvent << endl;
    cout << "total disk reads:   " << readNum << endl;
    cout << "total disk writes:  " << writeNum <<endl;
    cout << "page faults:        " << pageFaults << endl;
    cout << "prefetch faults:    " << (prefetchFaults) << endl;


}




















