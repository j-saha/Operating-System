#include<iostream>
#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include <fstream>
#include<cstring>
#include <sstream>
#include <unistd.h>
#include <ctime>
#include <chrono>
#include <cmath>
#include <random>
#include <cstdio>
#include <iterator>
#include <random>
using namespace std;
void * boarding(void * arg);
void * vip_channel_left_right(void * arg);
class Passenger;
class Airport;

sem_t kiosk_sem;
sem_t* sec_chk_sem;
sem_t right_left_mtx;
sem_t left_right_start_mtx;
vector<Passenger*> passenger;

Airport* airport;

int left_right_count=0;
int right_left_count=0;

pthread_mutex_t boarding_mtx, print_mtx, kiosk_id_mtx, left_right_count_mtx, special_kiosk_mtx, right_left_count_mtx;


class Airport{
    int kiosk_no, belt_no, pass_for_belt, w, x, y, z;
    bool* arr_kiosk; //true=occupied, false=unoccupied
    string output_filename;
public:
    Airport(int kiosk_no, int belt_no, int pass_for_belt, int w, int x, int y, int z, string output_filename="1705030.txt")
    {
        this->kiosk_no=kiosk_no;
        this->belt_no=belt_no;
        this->pass_for_belt=pass_for_belt;
        this->w=w;
        this->x=x;
        this->y=y;
        this->z=z;
        this->output_filename=output_filename;
        kiosk_init();
    }
    int get_kiosk_no(){return kiosk_no;}
    int get_belt_no(){return belt_no;}
    int get_pass_for_belt(){return pass_for_belt;}
    int get_self_check_sleep(){return w;}
    int get_security_sleep(){return x;}
    int get_boarding_sleep(){return y;}
    int get_vip_sleep(){return z;}
    string get_output_filename(){return output_filename;}

    void kiosk_init()
    {
        arr_kiosk=new bool[get_kiosk_no()];
        for(int i=0; i<get_kiosk_no(); i++)
        {
            arr_kiosk[i]=false;
        }
    }
    int occupy_kiosk()
    {


        for(int i=0; i<airport->get_kiosk_no(); i++)
        {

            if(arr_kiosk[i]==false)
            {
                arr_kiosk[i]=true;
                return i+1;
            }
        }
        return -99;

    }
    void unoccupy_kiosk(int kiosk_id)
    {
        arr_kiosk[kiosk_id-1]=false;
    }


};

class Passenger
{
    int id;
    bool vip;
    int time;
public:
    Passenger(int id, int start_time)
    {
        this->id=id;
        this->time=start_time;
        vip=false;
    }

    void set_vip_status()
    {
        vip=true;

    }
    void set_time(int start_time)
    {
        this->time=start_time;

    }
    void add_time(double time)
    {
        this->time+=round(time);

    }
    int get_time()
    {
        return time;
    }
    bool is_vip()
    {
        return vip;

    }
    string get_vip_str()
    {
        if(vip)
        {
            return " (VIP)";
        }
        return "";

    }

};


class Timer{
    std::chrono::high_resolution_clock::time_point t1;
    std::chrono::high_resolution_clock::time_point t2;


public:
    void start()
    {
        t1=std::chrono::high_resolution_clock::now();
    }
    void finish(int id)
    {
        t2=std::chrono::high_resolution_clock::now();
        this->add_duration(id);
    }
    void add_duration(int id)
    {
        std::chrono::duration<double> time_diff1 = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        passenger[id-1]->add_time(time_diff1.count());
    }


};
void input_from_file(string filename)
{
    string line;
    int arr[10];

    ifstream file(filename);
    getline (file, line);
    stringstream ssin(line);
    int i;
    for (i=0; ssin.good(); i++){
        ssin >> arr[i];
    }
    getline (file, line);
    stringstream ssin2(line);
    for (; ssin2.good(); i++){
        ssin2 >> arr[i];
    }
    airport=new Airport(arr[0],arr[1],arr[2],arr[3],arr[4],arr[5],arr[6]);
}

void semaphore_init()
{
    sec_chk_sem=new sem_t[airport->get_belt_no()];
    for(int i=0; i<airport->get_belt_no(); i++)
    {
        sem_init(&sec_chk_sem[i],0,airport->get_pass_for_belt());
    }
    sem_init(&kiosk_sem,0,airport->get_kiosk_no());

    pthread_mutex_init(&boarding_mtx,NULL);
    pthread_mutex_init(&print_mtx,NULL);
    pthread_mutex_init(&kiosk_id_mtx,NULL);
    pthread_mutex_init(&left_right_count_mtx,NULL);
    pthread_mutex_init(&special_kiosk_mtx,NULL);
    pthread_mutex_init(&right_left_count_mtx,NULL);
    sem_init(&right_left_mtx,0,1);
    sem_init(&left_right_start_mtx,0,1);

}



void print_text(string text)
{
    pthread_mutex_lock(&print_mtx);
    cout<<text<<endl;
    ofstream file;
    file.open(airport->get_output_filename(), std::ios_base::app);
    file<<text+"\n";
    file.close();
    pthread_mutex_unlock(&print_mtx);
}

void * special_kiosk(void * arg)
{
    int id;
    Timer timer;
    sscanf((char*)arg, "%d", &id);
    stringstream strs;
    strs << id;
    string temp_str = strs.str();
    char* ch_id = (char*) temp_str.c_str();

    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started waiting in the special kiosk at time "+to_string(passenger[id-1]->get_time()));
    timer.start();
    pthread_mutex_lock(&special_kiosk_mtx);
    timer.finish(id);

    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started self-check in special kiosk at time "+to_string(passenger[id-1]->get_time()));
    sleep(airport->get_self_check_sleep());
    passenger[id-1]->add_time(airport->get_self_check_sleep());
    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has finished self-check in special kiosk at time "+to_string(passenger[id-1]->get_time()));
    pthread_mutex_unlock(&special_kiosk_mtx);

	vip_channel_left_right((void*)ch_id);

    return nullptr;
}

void * vip_channel_right_left(void * arg)
{
    int id;
    Timer timer;
    sscanf((char*)arg, "%d", &id);
    stringstream strs;
    strs << id;
    string temp_str = strs.str();
    char* ch_id = (char*) temp_str.c_str();


    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" is waiting at VIP channel(right-left) at time "+to_string(passenger[id-1]->get_time()));
    timer.start();
    sem_wait(&right_left_mtx);
    timer.finish(id);
    sem_post(&right_left_mtx);

    pthread_mutex_lock(&right_left_count_mtx);

    right_left_count++;
    if(right_left_count==1)
    {
        sem_wait(&left_right_start_mtx);

    }
    pthread_mutex_unlock(&right_left_count_mtx);
    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started walking through VIP channel(right-left) at time "+to_string(passenger[id-1]->get_time()));
    sleep(airport->get_vip_sleep());
    passenger[id-1]->add_time(airport->get_vip_sleep());
    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has finished walking through VIP channel(right-left) at time "+to_string(passenger[id-1]->get_time()));



    pthread_mutex_lock(&right_left_count_mtx);
    right_left_count--;
    if(right_left_count==0)
    {
        sem_post(&left_right_start_mtx);
    }
    pthread_mutex_unlock(&right_left_count_mtx);


    special_kiosk((void*)ch_id);

    return nullptr;

}
void * vip_channel_left_right(void * arg)
{
    int id;
    Timer timer;
    sscanf((char*)arg, "%d", &id);
    stringstream strs;
    strs << id;
    string temp_str = strs.str();
    char* ch_id = (char*) temp_str.c_str();

    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" is waiting at VIP channel(left-right) at time "+to_string(passenger[id-1]->get_time()));
    pthread_mutex_lock(&left_right_count_mtx);
    left_right_count++;
    if(left_right_count==1) sem_wait(&right_left_mtx);
    pthread_mutex_unlock(&left_right_count_mtx);


    if(right_left_count>0)
    {
        timer.start();
        sem_wait(&left_right_start_mtx);
        sem_post(&left_right_start_mtx);
        timer.finish(id);
    }


    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started walking through VIP channel(left-right) at time "+to_string(passenger[id-1]->get_time()));
	sleep(airport->get_vip_sleep());
    passenger[id-1]->add_time(airport->get_vip_sleep());
    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has finished walking through VIP channel(left-right) at time "+to_string(passenger[id-1]->get_time()));


    pthread_mutex_lock(&left_right_count_mtx);
    left_right_count--;
    if(left_right_count==0) sem_post(&right_left_mtx);
    pthread_mutex_unlock(&left_right_count_mtx);


    boarding((void*)ch_id);

    return nullptr;

}


void * boarding(void * arg)
{

    int id;
    Timer timer;
    sscanf((char*)arg, "%d", &id);
    stringstream strs;
    strs << id;
    string temp_str = strs.str();
    char* ch_id = (char*) temp_str.c_str();

    bool lost_pass=false;
    if(rand()%2 == 0)
    {
        lost_pass=true;
    }

    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started waiting to be boarded at time "+to_string(passenger[id-1]->get_time()));
    timer.start();
    pthread_mutex_lock(&boarding_mtx);
    timer.finish(id);

    if(lost_pass)
    {
        print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has lost boarding pass at time "+to_string(passenger[id-1]->get_time()));
        pthread_mutex_unlock(&boarding_mtx);
        vip_channel_right_left((void*)ch_id);


    }
    else{
        print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started boarding the plane at time "+to_string(passenger[id-1]->get_time()));
        sleep(airport->get_boarding_sleep());
        passenger[id-1]->add_time(airport->get_boarding_sleep());
        print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has boarded the plane at time "+to_string(passenger[id-1]->get_time()));
        pthread_mutex_unlock(&boarding_mtx);
    }

	return nullptr;
}
void * security_check(void * arg)
{


    int id;
    Timer timer;
    sscanf((char*)arg, "%d", &id);
    stringstream strs;
    strs << id;
    string temp_str = strs.str();
    char* ch_id = (char*) temp_str.c_str();

    int belt_id = 1 + rand() % airport->get_belt_no();
    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started waiting for security check in belt "+to_string(belt_id)+" from time "+to_string(passenger[id-1]->get_time()));
    timer.start();
    sem_wait(&sec_chk_sem[belt_id-1]);
    timer.finish(id);

    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started the security check at time "+to_string(passenger[id-1]->get_time()));

    sleep(airport->get_security_sleep());
    passenger[id-1]->add_time(airport->get_security_sleep());
    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has crossed the security check at time "+to_string(passenger[id-1]->get_time()));

	sem_post(&sec_chk_sem[belt_id-1]);
	boarding((void*)ch_id);

	return nullptr;
}
void * kiosk(void * arg)
{
    int id, kiosk_id;
    Timer timer;
    sscanf((char*)arg, "%d", &id);
    stringstream strs;
    strs << id;
    string temp_str = strs.str();
    char* ch_id = (char*) temp_str.c_str();


    timer.start();
    sem_wait(&kiosk_sem);
    timer.finish(id);

    pthread_mutex_lock(&kiosk_id_mtx);
    kiosk_id=airport->occupy_kiosk();
    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has started self-check in at kiosk "+to_string(kiosk_id)+" at time "+to_string(passenger[id-1]->get_time()));
    pthread_mutex_unlock(&kiosk_id_mtx);


    sleep(airport->get_self_check_sleep());

    passenger[id-1]->add_time(airport->get_self_check_sleep());


    pthread_mutex_lock(&kiosk_id_mtx);
    airport->unoccupy_kiosk(kiosk_id);
    print_text("Passenger "+temp_str+passenger[id-1]->get_vip_str()+" has finished self-check in at kiosk "+to_string(kiosk_id)+" at time "+to_string(passenger[id-1]->get_time()));
    pthread_mutex_unlock(&kiosk_id_mtx);


	sem_post(&kiosk_sem);


	if(passenger[id-1]->is_vip())
    {
        vip_channel_left_right((void*)ch_id);
    }
	else security_check((void*)ch_id);

	return nullptr;
}



int main()
{
    srand(time(0));
    std::random_device rand_dev;
    std::mt19937 random_generator (rand_dev ());

    double mean = 4;
    double lamda = 1 / mean;
    std::exponential_distribution<double> exp_dis (lamda);


    string filename="input.txt";
    input_from_file(filename);

    char file_out[1024];
    strcpy(file_out, airport->get_output_filename().c_str());
    remove(file_out);

    string text;
    semaphore_init();




    int maximum_student=500, current_student=20;
    pthread_t* students = new pthread_t[maximum_student];

    int time=0;
    int stu_count=0;

    while(true)
    {
        int sleep_time = exp_dis.operator() (random_generator);
        sleep(sleep_time);
        time=time+sleep_time;


        Passenger* p = new Passenger(stu_count+1, time);

        if(rand()%2 == 0)
        {
            p->set_vip_status();
        }
        passenger.push_back(p);
        stringstream strs;
        strs << (stu_count+1);
        string temp_str = strs.str();
        char* id = (char*) temp_str.c_str();



        print_text("Passenger "+temp_str+passenger[stu_count]->get_vip_str()+" arrived at airport at "+to_string(passenger[stu_count]->get_time()));
        pthread_create(&students[stu_count], NULL, kiosk, (void*)id);

        stu_count++;
        if(stu_count==current_student) break;

    }
    while(1);
    return 0;
}


