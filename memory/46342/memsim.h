#include<cstring>
#include<iostream>
#include<vector>
using namespace std;

//解析每行的数据，w or r and address
int parse(string line, string &type, long* address, int pageSize){
    string* res = new string[2];
    int len = strlen(line.c_str());
    char temp[len+1];
    line.copy(temp,len);
    temp[len] = '\0';

    //用strtok吧字符串按照空格分割
    char* p;
    p = strtok(temp," ");

    int i=0;
    while(p){
        if(strcmp(p,"#")==0){
            p = strtok(NULL," ");
            sscanf(p,"%ld",address);
            return 1;
        }
        //保存数据
        res[i] = p;
        p = strtok(NULL," ");
        i++;

    }

    type = res[0];
    sscanf(res[1].c_str(),"%lx",address);

    //根据page size 转换成page id
    long pageid = (long)((*address)/pageSize);
    *address = pageid;
    
    return 0;


}

//fifo模式下所需要的节点
struct Fifo{
    long address;
    int pageId;
    Fifo* next;
};

//队列节点
struct Node{
    int pageId;
    int dirty;
    unsigned int shiftRegister;
    Node* next;
};
//队列
class Queue{
    public:
        Node* head;
        int len;
    public:

        Queue(int len){
            head = NULL;
            this->len = len;
            
        }
        //判断id 是否存在
        int exist(int id){
            Node *p = head;
            int i=0;
            while(p){
                if(p->pageId == id){
                    return i;
                }
                i++;
                p = p->next;
            }

            return -1;
        }
        //对队列中的制定id的dirty字段复制，此时说明该page已经是dirty状态
        void markDirty(int id){
            Node* p;
            p = head;
            while(p){
                if(p->pageId == id){
                    p->dirty = 1;
                    break;
                }
                p = p->next;
            }
        }
        //将一个page添加到队列里
        void add(int id,string scheme, int dirty){
            //如果队列空，那么
            if(head == NULL){

                Node* temp = new Node();
                temp->next = NULL;
                temp->pageId = id;
                temp->dirty = 1 ?(scheme.compare("W")==0) : dirty;
                temp->shiftRegister = 0;
                head = temp;
                return ;
            }
            //遍历所有节点，p指向最后一个节点
            Node* p= head;
            while(p->next){
                p= p->next;
            }
            //追加一个节点在末尾
            Node* temp = new Node();
            temp->next = NULL;
            temp->pageId = id;
            temp->dirty = 1 ?(scheme.compare("W")==0) : dirty;;
            temp->shiftRegister = 0;
            p->next = temp;
            
        }
        //从队列里删除id page，返回该节点
        Node* remove(int id){
            Node* p,*q;

            if(head == NULL){
                return NULL;
            }


            if(head->pageId == id){
                p = head;
                head = p->next;
    
                return p;
            }


            p = head->next;
            q = head;

            while(p->next){
                if(p->pageId == id){
                    break;
                }
                q = q->next;
                p = p->next;
            }

            q->next = p->next;

            return p;
        }

        //从队列里删除第一个元素
        int popFirst(){
            Node* p;

            if(head == NULL){
                return -1;
            }

            p = head->next;

            delete head;
            head = p;
            return 1;
        }

        //删除最后一个元素
        int removeBack(){
            Node* p;
            if(head == NULL){
                return -1;
            }

            p = head;

            while(p->next){
                p = p->next;
            }

            delete p;

            return 1;
        }
        //根据shitregister所记录的时间删除掉值最小的节点
        Node* removeLeast(){
            unsigned int min = 1000;

            Node* p,*q;
            p = head;
            //便利节点，找到哪个节点的shitfregister值最小
            while(p){
                if(p->shiftRegister < min){
                    min = p->shiftRegister;
                }
                p = p->next;
            }


            p = head;
            q = p;
            //删除值最小的节点
            while(p){
                int tempMin = p->shiftRegister;
                if( tempMin == min){
                    q = p->next;

                    if(q == NULL){
                        return p;
                        break;
                    }
                    Node* pt = new Node();
                    pt->shiftRegister = p->shiftRegister;
                    pt->pageId = p->pageId;
                    pt->dirty = p->dirty;
                    
                    p->shiftRegister = q->shiftRegister;
                    p->pageId = q->pageId;
                    p->dirty = q->dirty;
                    p->next = q->next;
     
                    delete q;
                    return pt;
                }else{
                    p = p->next;
                }   
            }
            return NULL;

        }

        //找到address的节点
        Node* find(int address){
            Node* p;
            p= head;
            while(p){
  
                if(p->pageId == address){
                    return p;
                }
                p = p->next;
            }

            return NULL;
        }
        //对每个元素的shiftregister值更新，针对arb和wsarb
        void setOne(int address){
            Node*p;
            p = head;

            while(p){
                if(p->pageId == address){
                    p->shiftRegister = p->shiftRegister | 0x80;
                }

                p = p->next;
            }
        }

        //更新每个节点的shiftregister 值
        void update(){

            Node* p;
            p = head;
            while(p){   
                    p->shiftRegister =(p->shiftRegister >> 1);
                
                    p->shiftRegister = p->shiftRegister & 0x7f;
                p = p->next;
            }

        }

};
//FIFO模式下的数据结构
class FIFO{

    public:
        bool debug;
        int pageSize;
        int pageNum;
        int currNum;
        
        Fifo* head;
        Queue* Q;
    
    public:
        FIFO(bool debug, int pageNum);
        int handle(long address,string scheme,int* readNum, int* writeNUm);
};

//arb模式下的数据结构对象
class ARB{
    public:
        bool debug;
        int pageSize;
        int pageNum;
        int currNum;
        int interval;
        unsigned int shiftRegister;
        Queue* Q;
        
    
    public:
        ARB(bool debug, int pageNum, int interval);
        int handle(long address,string scheme,int* readNum, int* writeNUm);
        void removeLRU();

};


//page对象
struct Page{
    int id;
    int dirty;
    unsigned int time;
};

//process对象
class Process{
    public:
    //每个对象保存一个set集合 针对wsarb模式
        vector<Page*> * set;
        int id;
        string name;
        int windowSize;
    public:
        //获取当前process的set
        vector<Page*>* getSet(){
            vector<Page*>* temp = new vector<Page*>();
            for(int i=set->size()-1;i>0;i--){
                int f = -1;
                for(int j=0;j<temp->size();j++){
                    if(temp->at(j)->id == set->at(i)->id){
                        f = j;
                    }
                }
                if(f==-1){
                    temp->push_back(set->at(i));
                }
            }
            return temp;
        }
        //添加一个page到set里
        int addPage(int id){
        
            if(set->size() >= windowSize){
                set->erase(set->begin());
            }
            Page* temp = new Page();
            temp->id = id;
            temp->dirty = 0;

            set->push_back(temp);
        }

};

//wsarb模式的数据结构
class WSARB{
    public:
        bool debug;
        int pageSize;
        int pageNum;
        int interval;
        int currNum;
        int processNum;
        int windowSize;
        Process* process;
        //保存的process集合
        vector<Process*> PS;
        //保存的pages集合
        vector<Page*> Pages;
    public:
        WSARB(bool debug, int pageNum,int interval,int windowSize);
        //找到一个page
        int findPage(int id){
            int flag=-1;
            for(int i=0;i<Pages.size();i++){
                if(Pages.at(i)->id == id){
                    flag = i;
                    break;
                }
            }
            return flag;
        }
        
        //对指定id的page设置dirty
        int markDirty(int id){
            for(int i=0;i<Pages.size();i++){
                if(Pages.at(i)->id== id){
                    Pages.at(i)->dirty = 1;
                    break;
                }
            }
        }
        //删除最早的page
        Page* removeLeast(){
            int min = 10000;
            int index = -1;

            Page* res = new Page();
            //找到time最小的节点，和arb一样
            for(int i=0;i<Pages.size();i++){
                if(Pages.at(i)->time<min){
                    min = Pages.at(i)->time;
                    index = i;
                }
            }
            //如果存在最小的话，那么删掉该节点
            if(index != -1){
                Page* t = Pages.at(index);
                res->id = t->id;
                res->dirty = t->dirty;
                res->time = t->time;

                //res = Pages.at(index);
                Pages.erase(Pages.begin()+index);
            }
            return res;
        }
        //添加一个page
        Page* addPage(int id){
            int flag= findPage(id);
            int  dirty = 0;

            Page* res = NULL;

            Page* temp  = new Page();
            temp->id= id;
            temp->time = 0;
            //如果内存中存在该节点
            if(flag != -1){
                //更新状态就好
                dirty = Pages.at(flag)->dirty;
                temp->time = Pages.at(flag)->time;
                Pages.erase(Pages.begin()+flag);
            }
            
            temp->dirty = dirty;
            Pages.push_back(temp);


            process->addPage(id);

            return res;

        }
        //进程切换时预取page
        int prepaging(int* pefectFaults,int* readNum,int* writeNum){
            vector<Page*>* pre = process->getSet();

            
            int temp;
            for(int i=0;i<pre->size();i++){
                temp = pre->at(i)->id;
                //不存在
                if(findPage(temp)==-1){
                    //发生prepage faluts
                    Page* t = removeLeast();
                    if(t->dirty == 1){
                        if(debug){
                                cout << "REPLACE:  page " << t->id<< " (DIRTY)" << endl;
                        }
                        (*writeNum) ++ ;
                    }else{
                        if(debug){
                            cout << "REPLACE:  page " << t->id << endl;
                        }
                    }
                    //添加到内存中
                    addPage(temp);
                    setOne(temp);
                    (*pefectFaults) ++ ;
                    (*readNum) ++ ;
                }
            }
        }
        //进程切换时，上下文转换
        void switchContext(int id,int *readNum, int* writeNum, int* pageFaults,int* pefetchFaults){
            int flag = -1;
            process = NULL;
            for(int i=0;i<PS.size();i++){
                if(PS.at(i)->id == id){
                    flag = 1;
                    process = PS.at(i);
                    break;
                }
            }
            //如果进程列表中存在该进程，那么进行页面预先读取
            if(process != NULL){
                prepaging(pefetchFaults,readNum,writeNum);
            }else{
                //不存在的花添加该进程
                process = new Process();
                process->id = id;
                process->name = "";
                process->windowSize = windowSize;
                process->set = new vector<Page*>();
                PS.push_back(process);

            }

        }
        //对addrss的page 更新time， 针对arb模式
        int setOne(long address){
            for(int i=0;i<Pages.size();i++){
                if(Pages.at(i)->id == address){
                   // Pages.at(i)->time = (Pages.at(i)->time >> 1);
                    Pages.at(i)->time = (Pages.at(i)->time | 0x80);
                    break;
                }
            }
        }
        //更新每个page的time，左移 一位
        int update(){
            for(int i=0;i<Pages.size();i++){
                
                Pages.at(i)->time = (Pages.at(i)->time >> 1) & 0x7f;
                Pages.at(i)->time = (Pages.at(i)->time & 0x7f);
            }
        }
        //处理每个内存请求
        int handle(long address,string scheme,int* readNum,int* writeNum){


            int index = findPage(address);
 
            //如果存在于内存
            if(index == -1){
                if(debug == true){ 
                    cout << "MISS:     page " << address << endl;
                }

                (*readNum) ++;
                //如果还有可以使用内存
                if(Pages.size() < pageNum){
                    addPage(address);
                    setOne(address);

                }else{
                    //删掉最久没用的内存，页面替换
                    Page* temp = removeLeast();

                    if(temp->dirty == 1){
                        if(debug){
                            cout << "REPLACE:  page " << temp->id<< " (DIRTY)" << endl;
                        }
                        (*writeNum) ++ ;
                    }else{
                        if(debug){
                            cout << "REPLACE:  page " << temp->id << endl;
                        }
                    }
                    //同ARB
                    addPage(address);
                    setOne(address);
                }

                if(scheme.compare("W") == 0){
                     markDirty(address);
                }

            }else{
                //如果说mode是w，那么说需要设置dirty
                if(scheme.compare("W") == 0){
                    markDirty(address);
                }
                if(debug == true){
                    cout << "HIT:      page " << address << endl;
                    
                }
                process->addPage(address);
                setOne(address);
            }
            return index;
        
        }

};

