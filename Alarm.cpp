#include <iostream>
#include <thread>
#include <termios.h>  
#include <unistd.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <iomanip> 
#include <mutex> 

using namespace std;

char readBuffer[] = {0, 0, 0, 0}; // массив для чтения 
char writeBuffer[1] = {'3'};   
char writeBuffer2[1] = {'4'};
char writeBuffer3[1] = {'7'}; 
int m_filePort; // дескриптор файла порта
int m_filePortCl; // хранит возврат при закрытии файла
int Data;
class Connection
{
public:
    void ComInit()
    {
    m_filePort = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);  
    if (m_filePort < 0)
    {
        cout << "Error in opening COM" << endl;
    }
    struct termios options;                                // структура настроек
    tcgetattr(m_filePort, &options);                       // получение настроек порта    
    cfsetispeed(&options,B9600);                           // скорость приема
    cfsetospeed(&options,B9600);                           // скорость передачи
    options.c_cflag &= ~PARENB; 			     // откл.бит четности			
    options.c_cflag &= ~CSTOPB; 			     // стоп-бит 1			
    options.c_cflag &= ~CSIZE;                             // очистка маски размера данных
    options.c_cflag |=  CS8;				     // 8-бит размер данных		
    options.c_cflag &= ~CRTSCTS;			     // откл.управл. данными			
    options.c_oflag &= ~OPOST;                             // откл.режим ввода
    options.c_cflag |= CREAD | CLOCAL;                     // откл.линии модема
    options.c_iflag &= ~(IXON | IXOFF | IXANY);            // откл.управл.потоком данных 
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);    // неканонический режим
    options.c_cc[VMIN]  = 40;                              
    options.c_cc[VTIME] = 10;     
          
    if ((tcsetattr(m_filePort,TCSANOW,&options))!=0)
    { 
        cout << "Error setting" << endl;
    }
    else
        cout << "COM is connected" << endl;
    }
    
};

class ComPort
{
public:
    virtual void Receive() = 0;           
};

class SimpleMessage : public ComPort
{
public:
    void Receive() override  // прием целой величины           
    {   
        int countCycleRead; 
        for (countCycleRead = 0; countCycleRead < 3; countCycleRead++)
        {
            tcflush(m_filePort, TCIFLUSH);
            read(m_filePort,&readBuffer,4);
        }
    }
    void ReceiveRaw(mutex &m) //прием ряда целых чисел
    {
        for (int i = 0; i <= 3; i++)
        {
        write(m_filePort,&writeBuffer3,1);
        tcflush(m_filePort, TCIFLUSH);   
        m.lock();
        read(m_filePort,&readBuffer,4);   
        Data = readBuffer[0];
        cout << setw(30) << "Raw: " << Data << endl;
        m.unlock();
        this_thread::sleep_for(chrono::milliseconds(1000));
        }
    }   
};

class FloatMessage : public ComPort
{
public:
    void Receive() override  // прием величины с плавающей точкой             
    {   
        int countCycleRead; 
        for (countCycleRead = 0; countCycleRead < 3; countCycleRead++)
        {
            tcflush(m_filePort, TCIFLUSH);
            read(m_filePort,&readBuffer,4);
            readBuffer[2] = readBuffer[1];
            readBuffer[1] = ',';
        }
    }   
};

class Filter
{
private:
    int valueT;
    
public:
    Filter(int valueT)
    {
        this -> valueT = valueT;
    }
        
    void Alarm(int &a, mutex &m)
    {
        for (int i=1; i <= 10; i++)
        {
            m.lock();
        if (valueT < a)
        {
            cout << setw(0) << "Normal" << endl;
        }
            else cout << setw(0) << "Alarm" << endl;
        m.unlock();
        this_thread::sleep_for(chrono::milliseconds(1000));
        }
    } 
};

int main()
{    
    SimpleMessage simpleMessage;
    FloatMessage floatMessage;
    Connection connection;
    connection.ComInit();
    Filter filter(30);
    char key[2];
    mutex m;
   
    while (true) 
    { 
        cin >> key;
        if (key[0] == 'q') break;
        
        if (key[0] == 'c')
        {
            write(m_filePort,&writeBuffer,1);   
            simpleMessage.Receive(); 
            cout << readBuffer << endl;          
        } 
        
        if (key[0] == 't')
        {
            write(m_filePort,&writeBuffer2,1);  
            floatMessage.Receive();  
            cout << readBuffer << endl;
        } 
        
        if (key[0] == 'r')
        {
            thread t1(&SimpleMessage::ReceiveRaw, &simpleMessage, ref(m));
            thread t2(&Filter::Alarm, &filter, ref(Data), ref(m));
            t1.join(); 
            t2.join();   
        } 
    }   
                    
    if ((m_filePortCl = close(m_filePort)) == 0)    //закрываем порт
    {
        cout << "COM is closed" << endl;
    }
    else
        cout << "Error COM connecting" << endl;
           
    return 0;
}
