#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <fstream>

using namespace std;

const int GENERATION = 100;//迭代次数
const int NP = 30;//种群规模
const int LENTH = 15;//个体编码长度，即调度路线长度，也是维度
const int ZONES = 5;//区域数量，包含调度中心1，出发区域2，3，到达区域4，5
const int T = 60;//总调度时间
const int C_MAX = 20;//调度车容量
const double td  = 0.2;//装卸一辆单车的时间
const double F_scale = 1;//缩放因子
const double CR = 0.6;//交叉概率因子

struct individual//个体结构体
{
    int plan[LENTH] = { 0 };//存储调度方案编码的数组
    double  t[LENTH] = { 0 };//存储每个调度的开始时刻
    int C[LENTH] = { 0 };//每次调度开始时，调度车上的单车数量
    double n[ZONES][LENTH] = { 0 };//每次调度开始时，该区域的单车数量
    double B[ZONES][LENTH] = { 0 };//每次调度开始时，该区域的调度需求
    double b[LENTH] = { 0 };//每次调度实际装卸的单车数量
    int Qsum = 0;//调度车在整个调度过程中的总装卸量

};
individual Dispatch[ NP ];//种群结构体数组


double Times[ZONES][ZONES] =//区域间最短通行时间表
{
    {0,6,12,10,5},
    {6,0,8,7,6},
    {12,8,0,4,7},
    {10,7,4,0,5},
    {5,6,7,5,0}
};

const int Vehicles[ZONES][3] =//各区域初始车辆及车辆变化，第123列分别表示，初始车辆qi，调度时间段的车辆变化量Qi，调度结束预期维持的车辆数Fi
{
    {0,0,0},
    {10,-40,0},
    {15,-25,0},
    {10,40,20},
    {10,30,15}
};

void initPopulation(int NP,int ZONES,int LENTH);//初始化种群
void get_Individualdata(individual*);//调度过程数据计算
individual* mutation ( individual Dispatch[]);//变异函数
individual* cross(individual v_individual[], individual Dispatch[]);//杂交函数

int main()
{
    initPopulation(NP, ZONES, LENTH);
    get_Individualdata(Dispatch);
    individual* v_individual=NULL;
    individual* u_individual=NULL;
    int G = 1;
    while (G<=100)
    {
        v_individual = mutation(Dispatch);//变异
        u_individual = cross(v_individual, Dispatch);//杂交
        for (int i = 0; i < NP; i++)
        {
            if (u_individual[i].Qsum > Dispatch[i].Qsum)//选择
            {
                for (int j = 1; j < LENTH - 1; j++)
                {
                    Dispatch[i].plan[j] = u_individual[i].plan[j];
                }
            }

        }
        get_Individualdata(Dispatch);
        cout << G << " ";
        for (int i = 0; i < NP; i++)//test
        {
            cout<< Dispatch[i].Qsum << " ";
        }
        cout << "\n" << endl;
        G++;
    }

    return 0;
}
void initPopulation(int NP, int ZONES, int LENTH)
{
    srand((unsigned)time(NULL));

    for(int i = 0; i < NP; i++)
    {
        Dispatch[i].plan[0] = 1;//起点终点均为调度中心，且起始时间为0
        Dispatch[i].plan[LENTH-1] = 1;
        Dispatch[i].t[0] = 0;

        for (int j = 1; j < LENTH-1; j++)//生成初始调度方案
        {
            Dispatch[i].plan[j] = (rand() % 4) + 2;//生成2-5的随机数
            if (Dispatch[i].plan[j] == Dispatch[i].plan[j - 1])//判断是否与前一位相同
                j--;
        }

    }
}
void get_Individualdata(individual* Dispatch)
{
    for (int k = 0; k < NP; k++)
    {
        individual* p = Dispatch+k;

        for (int j = 0; j < ZONES; j++)//初始化每个区域的初始单车数量与调度需求
        {
            p->n[j][0] = Vehicles[j][0];
            p->B[j][0] = 0;
            for (int i = 1; i < LENTH ; i++)
            {
                p->n[j][i] = 0;
                p->B[j][i] = 0;
            }
        }
        p->Qsum = 0;//初始化总装卸量与调度车上单车数量
        for (int j = 0; j < LENTH; j++)
        {
            p->b[j] = 0;
            p->C[j] = 0;
        }


        for (int j = 1; j < LENTH; j++)
        {
            p->t[j] = p->t[j - 1] + Times[p->plan[j - 1] - 1][p->plan[j] - 1] + td * abs(p->b[j - 1]);//计算每次调度的开始时刻
            if (p->t[j] - T >= 0)break;//判断是否超过调度总时间

            
            p->C[j] = p->C[j-1]+ p->b[j - 1];//更新每时刻的调度车上单车数量
            if (p->C[j] < 0)p->C[j] = 0;

            for (int i = 1; i < ZONES; i++)
            {
                double temp1 = (p->n[i][j - 1] + ((p->t[j] - p->t[j - 1]) * (Vehicles[i][1]) / T));//时刻更新每一区域的单车数量
                p->n[i][j] = round((temp1 > 0) ? temp1 : 0);

                double temp2 = round(p->n[i][j] + (T - p->t[j]) * (Vehicles[i][1]) / T);//时刻更新每一区域的调度需求
                if (i == 1 || i == 2)//出行区域
                {
                    p->B[i][j] = temp2;
                }
                else//到达区域
                {
                    p->B[i][j] = temp2 - Vehicles[i][2];
                }
            }
            if (p->B[p->plan[j]-1][j] < 0)//计算实际装卸单车数量
            {
                p->b[j] = ((-p->C[j]) > p->B[p->plan[j]-1][j]) ? (-p->C[j]) : p->B[p->plan[j]-1][j];
            }
            else
            {
                p->b[j] = ((C_MAX - p->C[j]) < p->B[p->plan[j]-1][j]) ? (C_MAX - p->C[j]) : p->B[p->plan[j]-1][j];
                if (p->b[j] > p->n[p->plan[j]-1][j])
                {
                    p->b[j] = p->n[p->plan[j]-1][j];
                }
            }
            p->n[p->plan[j]-1][j] -= p->b[j];//更新所在区域单车数量
            if (p->n[p->plan[j]-1][j] < 0)
                p->n[p->plan[j]-1][j] = 0;
            p->Qsum += abs(p->b[j]);//累加求和
        }
    }
}
individual* mutation(individual Dispatch[])
{
    individual* v_individual = new individual[NP];
    memcpy(v_individual, Dispatch, sizeof(individual[NP]));

    for (int i = 0; i < NP; i++)
    {
        int r1 = 0;
        int r2 = 0;
        int r3 = 0;
        
        for (int j = 1; j < LENTH-1; j++)
        {
            while (1)//随机选择三个下标不为i的互异个体
            {

                r1 = rand() % (NP);
                r2 = rand() % (NP);
                r3 = rand() % (NP);
                if (r1 != r2 && r1 != r3 && r2 != r3 && r1 != i && r2 != i && r3 != i)
                break;
            }

            v_individual[i].plan[j] = Dispatch[r1].plan[j] + F_scale * (Dispatch[r2].plan[j] - Dispatch[r3].plan[j]);//变异操作

            if (v_individual[i].plan[j] > 5 || v_individual[i].plan[j] < 2)//定义域合法性检查
            {
                j--;
            }


            else if (v_individual[i].plan[j] == v_individual[i].plan[j - 1] )//连续检查
            {
                j--;
            }

         } 
    }
    get_Individualdata(v_individual);//调度过程数据计算
    return v_individual;
}
individual* cross(individual v_individual[],individual Dispatch[])
{
    individual* u_individual = new individual[NP];
    memcpy(u_individual, Dispatch, sizeof(individual[NP]));
    for (int i = 0; i < NP; i++)
    {
        int jrand = rand() % 13 + 1;
        for (int j = 1; j < LENTH-1; j++)
        {
            float rand01 = rand() / (RAND_MAX + 1.0);
            if ((rand01> CR || j == jrand)//随机杂交
                && (v_individual[i].plan[j] != u_individual[i].plan[j - 1]) //前连续检查
                && (v_individual[i].plan[j] != u_individual[i].plan[j + 1]))//后连续检查
                u_individual[i].plan[j] = v_individual[i].plan[j];
        }
    }
    get_Individualdata(u_individual);
    return u_individual;
}

