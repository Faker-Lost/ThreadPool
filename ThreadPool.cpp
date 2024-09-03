#include"ThreadPool.h"
#include<iostream>
using namespace std;

ThreadPool::ThreadPool(int min, int max) :m_maxThread(max), m_minThread(min),
m_stop(false), m_freeThread(max), m_curThread(max)
{
    cout << "max : " << max << endl;
    //�����������߳� ���������������ɵ��ö���
    m_manager = new thread(&ThreadPool::manager, this);
    //���������߳�
    for (int i = 0; i < max; i++)
    {
        // ָ��������
        thread t(&ThreadPool::worker, this);
        // ���̶߳��������������
        m_workers.insert(make_pair(t.get_id(), move(t)));
    }
}

//�������� �ͷ���Դ
ThreadPool::~ThreadPool()
{
    m_stop = true;
    //���������������߳�
    m_condition.notify_all();
    for (auto& it : m_workers) //���ã��̶߳���������
    {
        thread& t = it.second;
        //�жϵ�ǰ�����Ƿ������
        if (t.joinable())
        {
            cout << "******** �߳� " << t.get_id() << " ��Ҫ�˳�..." << endl;
            t.join();
        }
    }
    if (m_manager->joinable())
    {
        m_manager->join();
    }

    delete m_manager;
}

//������� ������Դ���̴߳����������ȡ��Դ����Ҫ�����߳�ͬ��
void ThreadPool::addTask(function<void(void)> task)
{
    {
        //����������
        lock_guard<mutex> locker(m_queueMutex);
        m_task.emplace(task);
    }
    //���Ѻ���
    m_condition.notify_one();
}

//�������߳�
void ThreadPool::manager(void)
{
    // ��⹤�����̵߳��������������Ӻͼ���
    while (!m_stop.load())
    {
        //˯��3��
        this_thread::sleep_for(chrono::seconds(1));
        //��ȡ���е��̺߳��̳߳�����߳�
        int freeidel = m_freeThread.load();
        int cur = m_curThread.load();
        //�̵߳�ɾ��
        if (freeidel > cur / 2 && cur > m_minThread)
        {
            //�����߳�,ÿ�����������߳�
            m_deleteThread.store(2);
            m_condition.notify_all();
            //m_ids Ϊ������Դ
            lock_guard<mutex> ids_lock(m_idsMutex);
            for (auto id : m_ids)
            {
                auto it = m_workers.find(id);
                if (it != m_workers.end())
                {
                    cout << "======== �̣߳� " << (*it).first << "������..." << endl;
                    (*it).second.join();
                    m_workers.erase(it);
                }
            }
            m_ids.clear();
        }
        //�̵߳����
        else if (freeidel == 0 && cur < m_maxThread)
        {
            // ���̶߳��������������
            // ָ��������
            thread t(&ThreadPool::worker, this);
            cout << "+++++++++���һ���̣߳�id�� " << t.get_id() << endl;
            // ���̶߳��������������
            m_workers.insert(make_pair(t.get_id(), move(t)));
            m_curThread++;
            m_freeThread++;
        }
    }
}

//�������߳�
void ThreadPool::worker(void)
{
    //�ж���������Ƿ�Ϊ�� �գ�����
    while (!m_stop.load())
    {
        function<void(void)> task = nullptr;

        {
            unique_lock<mutex> locker(m_queueMutex);

            //�ж���������Ƿ�Ϊ��&& �Ƿ���̳߳�
            while (m_task.empty() && !m_stop)
            {
                //������ǰ�߳�
                m_condition.wait(locker);
                if (m_deleteThread.load() > 0)
                {
                    cout << "-----------�߳��˳���ID��" << this_thread::get_id() << endl;
                    m_curThread--;
                    m_freeThread--;
                    m_deleteThread--;
                    //�߳��˳��������˳��߳�id
                    //m_ids ������Դ����Ҫ����
                    lock_guard<mutex> ids_lock(m_idsMutex);
                    m_ids.emplace_back(this_thread::get_id());
                    return;
                }
            }

            //ȡ����
            if (!m_task.empty())
            {
                //���⿽��
                cout << "ȡ��һ������..." << endl;
                task = move(m_task.front());
                //����
                m_task.pop();
            }
        }

        //ִ������
        if (task)
        {
            m_freeThread--;
            task();
            m_freeThread++;
        }
    }
}
//�����ȴ�
void calc(int x, int y)
{
    int z = x + y;
    cout << "z = " << z << endl;
    this_thread::sleep_for(chrono::seconds(5));
}
//�õ�һ������ֵ������һ������
int calc1(int x, int y)
{
    int z = x + y;
    this_thread::sleep_for(chrono::seconds(5));
    return z;
}


int main()
{
    ThreadPool pool;
    vector<future<int>> result;
    for (int i = 0; i < 10; i++)
    {
        //auto obj = bind(calc, i, i * 2);
        result.emplace_back(pool.addTask(calc1, i, i*2));
    }
    for (auto& item : result)
    {
        cout << "�߳�ִ�еĽṹ��" << item.get() << endl;
    }
    //��֤���̲߳��˳�
    getchar();


    return 0;
}