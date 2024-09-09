#pragma once
#pragma once
#pragma once
#include<thread>
#include<vector>
#include<map>
#include<atomic>
#include<queue>
#include<functional>
#include<mutex>
#include<condition_variable>
#include<future>
#include<memory>
using namespace std;
/* �̳߳صĹ���:
* 1. �������߳�->���߳� 1��
*  - ���ƹ����̵߳����������ӻ����
* 2. ���ɸ��������߳�->���߳� n��
*   - �����������ȡ����
    - �������Ϊ�գ�������������������������
    - �߳�ͬ������������
    - ��ǰ�����̵߳�����,���е��߳�������
    - ָ�������С�߳�����
* 3. �������->queue
*   - ������
*   - ��������
* 4. �̳߳ؿ���->bool
*/
class ThreadPool
{
public:
    // 1 ��ʼ���̳߳� hardware_concurrency():��ȡ��ǰ�豸CPU������
    ThreadPool(int min = 2, int max = thread::hardware_concurrency());
    ~ThreadPool();
    // ������ -> �������
    void addTask(function<void(void)>);
     
    // ��Ҫ����ֵ ָ��һ�����,            ����ֵfuture���͵Ķ������� typename
    template<typename T, typename...Args>
    // T&& δ���������ͣ����ݴ���Ĳ��������Ƶ�
    auto addTask(T&& t, Args&&... args) -> future<typename result_of<T(Args...)>::type>
    {
        //����������ӵ��������
        //1 package_task �࣬����get_future 
        using returnType = typename result_of<T(Args...)>::type;
        //����ת�� �����Ӧ����������
         auto mytask = make_shared< packaged_task<returnType()> > (
                bind(forward<T>(t), forward<Args>(args)...)
            );
        //2 get future()
        future<returnType> res = mytask->get_future();
        //3 package_task ��ӵ��������
        // ��������
        m_queueMutex.lock();
        m_task.emplace(
            [mytask]() {
                (*mytask)();
            });
        m_queueMutex.unlock();
        //����
        m_condition.notify_one( );

        return res;
    }

private:
    void manager(void);
    void worker(void);
private:
    thread* m_manager;          //�������߳�
    map<thread::id, thread> m_workers;
    vector<thread::id> m_ids;   //�������߳�
    atomic<int> m_minThread;    //��С����
    atomic<int> m_maxThread;    //�������
    atomic<int> m_curThread;    //��ǰ�߳�����
    atomic<int> m_freeThread;    //�����߳����� 
    atomic<int> m_deleteThread; //�����߳�
    atomic<bool> m_stop;         //�̳߳ؿ���
    //����ָ�룬���ݺ���ָ�룬��������װ��Ŀɵ��ö���
    queue<function<void(void)>> m_task;
    //������
    mutex m_queueMutex;
    mutex m_idsMutex;
    //���������� �����������߳�
    condition_variable m_condition;
};