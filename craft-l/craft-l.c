#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <math.h>

#define KEY_DOWN 0402
#define KEY_UP 0403
#define KEY_LEFT 0404
#define KEY_RIGHT 0405

struct pos{
  int x;
  int y;
  int z;
};

struct item_info{
  int num;
  int chance;
  int fluc;
  int front_color;
  int back_color;
  int liquid;
  int cross;
  int get;
  int use;
  int depend[101];
  int depend_r[101];
  int depend_deep_r[101];
  int max_depend;
  char *symbol;
};

int item_n=3; //When you add new item,please change it
struct item_info item[4];
struct item_info grass;
struct pos position;
int stuff_now=0;
int map[401][401][201]={0};
int max_width=400;
int max_height=400;
int max_deep=200;
int min_width=1;
int min_height=1;
int min_deep=1;
int dstart_y=1;
int dstart_x=1;
int dstart_z=1;
int dstart_x_deep=1;
int row=0;
int col=0;
int stuff[101]={0};
int stuff_node[101]={0};
int max_stuff_node=100;
int max_stuff_vis=3;
int max_r_fluc=10;
int pos_old=0;
int display_mode=0;
int horizon=100;
int direct[6][3]={
  {0,-1,0}, //left
  {0,1,0}, //right
  {-1,0,0}, //forward
  {1,0,0}, //backward
  {0,0,-1}, //up
  {0,0,1}, //down
};

//Client Part
int display();
int get_pro();
int main_loop();
int init_all();
int init_digital();
int init_license();
void cleanup();
int GetRandom();
int proc(int code);
int w_col();
int random_stuff(int num,int chance,int fluc,int liquid);
int item_init();
int get_stuff(int code);
int right_touch(int x,int y,int z);
int liquid_dfs(int x,int y,int z,int num,int r,int max_r);
int check_x_y(int x,int y,int z);
int check_border(int x,int y,int z);
int check_x_y2(int x,int y,int z,int code);
int pull(int p);
int read_save();
int write_save();
int get_row_col();
int update_dstart();
int check_dstart_border();

//Server Part
int check_depend(int depend_num,int depend_each_r,int depend_each_deep,int x,int y,int z);
int server_landform(int num);
int encode(FILE *f_pointer,int i,int j);
int decode(FILE **f_pointer,int i,int j);
//End

int GetRandom()
{
  int rnum = 0;
  #if defined _MSC_VER
  #if defined _WIN32_WCE
    CeGenRandom(sizeof(int), (PBYTE)&rnum);
  #else
    HCRYPTPROV hProvider = 0;
    const DWORD dwLength = sizeof(int);
    BYTE pbBuffer[dwLength] = {};
    DWORD result =::CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    assert(result);
    result = ::CryptGenRandom(hProvider, dwLength, pbBuffer);
    rnum = *(int*)pbBuffer;
    assert(result);
    ::CryptReleaseContext(hProvider, 0);
  #endif
  #elif defined __GNUC__
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd != -1) {
      (void) read(fd, (void *) &rnum, sizeof(int));
      (void) close(fd);
    }
  #endif
  return rnum;
}

//Sever Part
int check_depend(int depend_num,int depend_each_r,int depend_each_deep,int x,int y,int z)
{
  int i=0,j=0,k=0;
  for (i=x-depend_each_r;i<=x+depend_each_r;i++)
    for (j=y-depend_each_r;j<=y+depend_each_r;j++)
      for (k=z-depend_each_deep;k<=z+depend_each_deep;k++)
        if (check_border(i,j,k)==0)
          if (map[i][j][k]==depend_num) return 1;
  return 0;
}

int server_landform(int num)
{
  int ran_x=0,ran_y=0,ran_z=0,i=0,flag=0,j=0;
  for (i=1;i<=item[num].chance+(abs(GetRandom())%item[num].fluc+1);i++)
  {
    while (1)
    {
    flag=0;
    ran_x=abs(GetRandom())%(max_height-horizon)+1+horizon;
    ran_y=abs(GetRandom())%(max_width-horizon)+1+horizon;
    ran_z=abs(GetRandom())%(max_deep-horizon)+1+horizon;
    for (j=1;j<=item[num].max_depend;j++)
    {
      flag=check_depend(item[num].depend[j],item[num].depend_r[j],item[num].depend_deep_r[j],ran_x,ran_y,ran_z);
    }
    if ((flag==1)&&(map[ran_x][ran_y][ran_z]==0))
    {
      map[ran_x][ran_y][ran_z]=item[num].num;
      break;
    }
    }
  }
  return 0;
}

int encode(FILE *f_pointer,int i,int j)
{
  int k=0,n=0,node=0;
  for (k=min_deep;k<=max_deep;k++)
  {
    if (map[i][j][k]!=map[i][j][n])
    {
      node+=2;
      n=k;
    }
  }
  node+=2;
  fprintf(f_pointer, "%d ", node);

  n=1;
  for (k=min_deep;k<=max_deep;k++)
  {
    if (map[i][j][k]!=map[i][j][n])
    {
      fprintf(f_pointer, "%d %d ", k-n, map[i][j][k-1]);
      n=k;
    }
  }
  fprintf(f_pointer, "%d %d ", k-n, map[i][j][k-1]);
  n=k;
  return 0;
}

int decode(FILE **f_pointer,int i,int j)
{
  int n=0,start=1,k=0,l=0,temp=0,num=0;
  fscanf(*f_pointer,"%d ",&n);
  for (k=1;k<=n;k++)
  {
    if (k%2==0)
    {
      fscanf(*f_pointer,"%d ",&temp);
      if (start+num-1>max_deep) return 1;
      for (l=start;l<=start+num-1;l++)
        map[i][j][l]=temp;
      start+=num;
      num=0;
    }
    else {fscanf(*f_pointer,"%d ",&num);}
  }
  return 0;
}

//Client Part
int check_border(int x,int y,int z)
{
  if ((x>max_height)||(x<min_height)) return 1;
  if ((y>max_width)||(y<min_width)) return 1;
  if ((z>max_deep)||(z<min_deep)) return 1;
  return 0;
}


int read_save()
{
  char path[100]={0};
  FILE *fp;
  int return_code=0,i=0,j=0,temp_width=0,temp_height=0,temp_deep=0,temp=0,num=0;
  erase();
  refresh();
  printw("[OPEN] Please input the path to open the save:\n");
  refresh();
  echo();
  scanw("%s",path);
  refresh();
  noecho();

  if ((fp=fopen(path,"r"))==NULL)
  {
    printw("[OPEN] Open save failed\n");
    getch();
    refresh();
    return 1;
  }
  fscanf(fp,"%d %d %d\n",&temp_height,&temp_width,&temp_deep);
  if ((temp_height==max_height)&&(temp_width==max_width)&&(temp_deep==max_deep))
    {max_height=temp_height;max_width=temp_width;max_deep=temp_deep;}
  else
  {
    printw("[OPEN] The map size is incorrect!\n");
    getch();
    refresh();
    return 2;
  }

  fscanf(fp,"%d %d %d\n",&position.x,&position.y,&position.z);
  fscanf(fp,"%d",&stuff_node[0]);
  if (stuff_node[0]>max_stuff_node) stuff_node[0]=max_stuff_node;
  for (i=1;i<=stuff_node[0];i++)
    fscanf(fp,"%d",&stuff_node[i]);
  fscanf(fp,"\n");

  for (i=1;i<=stuff_node[0];i++)
  {
    fscanf(fp,"%d %d",&temp,&num);
    if (temp<=100) stuff[temp]=num;
    fscanf(fp,"\n");
  }
  fscanf(fp,"\n");

  for (i=min_height;i<=max_height;i++)
  {
    for (j=min_width;j<=max_width;j++)
    {
      return_code=decode(&fp,i,j);
      if (return_code==1)
      {
        printw("[OPEN] Map info out of range!\n");
        printw("[OPEN] Press any key to exit the game.\n");
        refresh();
        getch();
        return 1;
      }
    }
  }
  get_row_col();
  dstart_x=position.x;
  dstart_y=position.y;
  dstart_z=position.z;
  printw("[OPEN] Open save successful!\n");
  refresh();
  getch();
  display();
  return 0;
}

int write_save()
{
  char path[100]={0};
  FILE *fp;
  int i=0,j=0;
  erase();
  refresh();
  printw("[SAVE] Please input the path to write the save:\n");
  refresh();
  echo();
  scanw("%s",path);
  refresh();
  noecho();

  if ((fp=fopen(path,"w"))==NULL)
  {
    printw("[SAVE] Write save failed\n");
    getch();
    refresh();
    return 1;
  }
  fprintf(fp,"%d %d %d\n",max_height,max_width,max_deep);
  fprintf(fp,"%d %d %d\n",position.x,position.y,position.z);
  fprintf(fp,"%d ",stuff_node[0]);

  for (i=1;i<=stuff_node[0];i++)
    fprintf(fp,"%d ",stuff_node[i]);
  fprintf(fp,"\n");

  for (i=1;i<=stuff_node[0];i++)
  {
    fprintf(fp,"%d %d",item[stuff_node[i]].num,stuff[item[stuff_node[i]].num]);
    fprintf(fp,"\n");
  }

  for (i=min_height;i<=max_height;i++)
  {
    for (j=min_width;j<=max_width;j++)
    {
        encode(fp,i,j);
        fprintf(fp,"\n");
    }
  }
  get_row_col();
  printw("[SAVE] Write save successful!\n");
  refresh();
  getch();
  display();
  return 0;
}

int pull(int p)
{
  int i=0;
  for (i=p;i<stuff_node[0];i++)
  {
    stuff_node[i]=stuff_node[i+1];
  }
  stuff_node[stuff_node[0]]=0;
  return 0;
}

int check_x_y2(int x,int y,int z,int code)
{
  if ((code==1)&&(map[x][y-1][z]!=0)) return 1;
  if ((code==2)&&(map[x][y+1][z]!=0)) return 1;
  if ((code==3)&&(map[x-1][y][z]!=0)) return 1;
  if ((code==4)&&(map[x+1][y][z]!=0)) return 1;
  if ((code==5)&&(map[x][y][z-1]!=0)) return 1;
  if ((code==6)&&(map[x][y][z+1]!=0)) return 1;
  return 0;
}

int check_x_y(int x,int y,int z)
{
  int flag[7]={0};
  int i=0,on_sum=6,check_sum=0;
  if (!(y-1>=min_width)) flag[1]=1;
  if (!(y+1<=max_width)) flag[2]=1;
  if (!(x-1>=min_height)) flag[3]=1;
  if (!(x+1<=max_height)) flag[4]=1;
  if (!(z-1>=min_deep)) flag[5]=1;
  if (!(z+1<=max_deep)) flag[6]=1;
  for (i=1;i<=6;i++)
    if (flag[i]==0)
    {
      on_sum--;
      //printf("%d %d %d %d %d\n",i,x,y,z,check_border(x,y,z));
      check_sum+=check_x_y2(x,y,z,i);
    }
  if ((on_sum<6)&&(check_sum==6-on_sum)) return 1;
  return 0;
}

int liquid_dfs(int x,int y,int z,int num,int r,int max_r)
{
  int direct_num=0,x1=0,y1=0,z1=0;
  if (r==max_r+1)
     return 0;
  else
  {
    while (1)
    {
      direct_num=abs(GetRandom())%6+1;
      x1=x+direct[direct_num-1][0];
      y1=y+direct[direct_num-1][1];
      z1=z+direct[direct_num-1][2];
      if (check_x_y(x,y,z)==1) break;
      if ((check_border(x1,y1,z1)==0)&&(map[x1][y1][z1]==0))
      {
        map[x1][y1][z1]=num;
        liquid_dfs(x1,y1,z1,num,r+1,max_r);
      }

    }
  }
  return 0;
}

int right_touch(int x,int y,int z)
{
  if (map[x][y][z]>=item[1].num)
  {

  }
  if ((map[x][y][z]==0)&&(stuff[item[stuff_node[stuff_now]].num]-1>=0)&&(stuff_now>0)&&(stuff_now<=max_stuff_vis)&&(stuff_now<=stuff_node[0]))
  {
    map[x][y][z]=item[stuff_node[stuff_now]].num;
    stuff[item[stuff_node[stuff_now]].num]--;
    if (stuff[item[stuff_node[stuff_now]].num]==0)
    {
      stuff_node[stuff_now]=0;
      pull(stuff_now);
      stuff_node[0]--;
      stuff_now=0;
    }
  }
  return 0;
}

int get_stuff(int code)
{
  int i=0,flag=0;
  int x=0,y=0,z=0;

  x=position.x+direct[code-1][0];
  y=position.y+direct[code-1][1];
  z=position.z+direct[code-1][2];
  if (check_border(x,y,z)==0)
  {
    if (map[x][y][z]>=item[1].num)
    {
      if (item[map[x][y][z]-item[1].num+1].get==1)
      {
        for (i=1;i<=stuff_node[0];i++)
        {
          if (item[stuff_node[i]].num==map[x][y][z])
          {
            flag=1;
            stuff[item[stuff_node[i]].num]++;
            map[x][y][z]=0;
            break;
          }
        }
        if ((flag==0)&&(stuff_node[0]+1<=max_stuff_node))
        {
          stuff_node[0]++;
          stuff_node[stuff_node[0]]=map[x][y][z]-item[1].num+1;
          stuff[map[x][y][z]]++;
          map[x][y][z]=0;
        }
      }
      i=0;
    }
  }

  return 0;
}

int item_init()
{

  item[2].num=8;
  item[2].chance=1;
  item[2].fluc=200;
  item[2].symbol="'";
  item[2].front_color=COLOR_WHITE;
  item[2].back_color=COLOR_BLUE;
  item[2].liquid=1;
  item[2].cross=1;//can't
  item[2].get=0;//can
  item[2].use=0;

  item[1].num=7;
  item[1].chance=1000;
  item[1].fluc=100;
  item[1].symbol="#";
  item[1].front_color=COLOR_WHITE;
  item[1].back_color=COLOR_YELLOW;
  item[1].liquid=0;
  item[1].cross=0;//can't
  item[1].get=1;//can
  item[1].use=0;
  item[1].use=1;
  item[1].max_depend=1;
  item[1].depend[1]=item[2].num;
  item[1].depend_r[1]=30;
  item[1].depend_deep_r[1]=5;

  item[3].num=9;
  item[3].chance=2000;
  item[3].fluc=1000;
  item[3].symbol="*";
  item[3].front_color=COLOR_BLACK;
  item[3].back_color=COLOR_WHITE;
  item[3].liquid=0;
  item[3].cross=0;
  item[3].get=1;
  item[3].use=0;
  return 0;
}

int random_stuff(int num,int chance,int fluc,int liquid)
{
  int i=0,ran_x=0,ran_y=0,ran_z=0,flag=0;
  flag=0;
  if (liquid==0)
  {
    for (i=1;i<=(chance+(abs(GetRandom())%fluc));i++)
    {
      flag=0;
      while (flag==0)
      {
        ran_x=0;ran_y=0,ran_z=0;
        ran_x=abs(GetRandom())%(max_height-horizon)+1+horizon;
        ran_y=abs(GetRandom())%(max_width-horizon)+1+horizon;
        ran_z=abs(GetRandom())%(max_deep-horizon)+1+horizon;
        if ((ran_x!=position.x)&&(ran_y!=position.y)&&(ran_z!=position.z)&&(map[ran_x][ran_y][ran_z]==0))
        {
          flag=1;
          map[ran_x][ran_y][ran_z]=num;
        }
      }
    }
  }
  flag=0;
  if (liquid==1)
  {
    for (i=1;i<=(chance+(abs(GetRandom())%fluc));i++)
    {
      flag=0;
      while (flag==0)
      {
        ran_x=0;ran_y=0,ran_z=0;
        ran_x=abs(GetRandom())%(max_height-horizon)+1+horizon;
        ran_y=abs(GetRandom())%(max_width-horizon)+1+horizon;
        ran_z=abs(GetRandom())%(max_deep-horizon)+1+horizon;
        if ((ran_x!=position.x)&&(ran_y!=position.y)&&(ran_z!=position.z)&&(map[ran_x][ran_y][ran_z]==0))
        {
          flag=1;
          map[ran_x][ran_y][ran_z]=num;
          liquid_dfs(ran_x,ran_y,ran_z,num,1,35);
        }
      }
    }
  }
  return 0;
}

int get_row_col()
{
  struct winsize info;
  ioctl(STDIN_FILENO,TIOCGWINSZ,&info);

  if ((row!=info.ws_row)||(col!=info.ws_col))
  {
    dstart_x=position.x;
    dstart_y=position.y;
    if (display_mode!=1) dstart_z=position.z;
  }
  row=info.ws_row;
  col=info.ws_col;
  check_dstart_border();

  return 0;
}

int check_dstart_border()
{
  if (dstart_x+row-2>max_height) dstart_x=max_height-row+2;
  if (dstart_y+col-1>max_width) dstart_y=max_width-col+1;
  if (dstart_z+row-2>max_deep) dstart_z=max_deep-row+2;
  if (dstart_x_deep+col-1>max_width) dstart_x_deep=max_width-col+1;
  return 0;
}

int update_dstart()
{
  if (position.y<dstart_y) dstart_y--;
  if (position.y>(dstart_y+(col-1))) dstart_y++;
  if (position.x<dstart_x) dstart_x--;
  if (position.x>(dstart_x+(row-2))) dstart_x++;
  if (position.z<dstart_z) dstart_z--;
  if (position.z>(dstart_z+(row-2))) dstart_z++;
  if (position.x<dstart_x_deep) dstart_x_deep--;
  if (position.x>(dstart_x_deep+(col-1))) dstart_x_deep++;
  check_dstart_border();
  return 0;
}

int display()
{
  int i=0,j=0,k=0,n=0,i_start=0,i_end=0,j_start=0,j_end=0,k_start=0,k_end=0;
  erase();
  refresh();

  check_dstart_border();
  if (display_mode==1) //x-y mode
  {
    i_start=dstart_x;
    i_end=dstart_x+(row-2);
    j_start=dstart_y;
    j_end=dstart_y+(col-1);
    k=position.z;
    for (i=i_start;i<=i_end;i++)
      for (j=j_start;j<=j_end;j++)
        {
        if (map[i][j][k]==1) printw("▢");
        if (map[i][j][k]==0) printw(" ");
        for (n=1;n<=item_n;n++)
          if (map[i][j][k]==item[n].num)
          {
            init_pair(item[n].num,item[n].front_color,item[n].back_color);
            attron(COLOR_PAIR(item[n].num));
            printw("%s",item[n].symbol);
            attroff(COLOR_PAIR(item[n].num));
          }
      }
  }

  if (display_mode==2) //y-z mode
  {
    j_start=dstart_y;
    j_end=dstart_y+(col-1);
    k_start=dstart_z;
    k_end=dstart_z+(row-2);
    i=position.x;
    for (k=k_start;k<=k_end;k++)
      for (j=j_start;j<=j_end;j++)
      {
        if (map[i][j][k]==1) printw("▢");
        if (map[i][j][k]==0) printw(" ");
        for (n=1;n<=item_n;n++)
          if (map[i][j][k]==item[n].num)
          {
            init_pair(item[n].num,item[n].front_color,item[n].back_color);
            attron(COLOR_PAIR(item[n].num));
            printw("%s",item[n].symbol);
            attroff(COLOR_PAIR(item[n].num));
          }
      }
  }

  if (display_mode==3) //x-z mode
  {
    i_start=dstart_x_deep;
    i_end=dstart_x_deep+(col-1);
    k_start=dstart_z;
    k_end=dstart_z+(row-2);
    j=position.y;
    for (k=k_start;k<=k_end;k++)
      for (i=i_start;i<=i_end;i++)
        {
          if (map[i][j][k]==1) printw("▢");
          if (map[i][j][k]==0) printw(" ");
          for (n=1;n<=item_n;n++)
            if (map[i][j][k]==item[n].num)
            {
              init_pair(item[n].num,item[n].front_color,item[n].back_color);
              attron(COLOR_PAIR(item[n].num));
              printw("%s",item[n].symbol);
              attroff(COLOR_PAIR(item[n].num));
            }
        }
  }

  refresh();
  move(row-1,0);refresh();
  //strat color(black,white)
  init_pair(1,COLOR_BLACK,COLOR_WHITE);
  attron(COLOR_PAIR(1));

  printw("X:%d Y:%d Z:%d row:%d col:%d D-M:%d",position.x,position.y,-(position.z-horizon),row,col,display_mode);refresh();
  if (stuff_now>0)
    printw(" N:%s",item[stuff_node[stuff_now]].symbol);
  else printw(" N:Hand");
  printw(" S(%d):",stuff_node[0]);
  if (stuff_node[0]<max_stuff_vis)
  {
    for (i=1;i<=stuff_node[0];i++)
      printw("%d %s ",stuff[item[stuff_node[i]].num],item[stuff_node[i]].symbol);refresh();
  }
  if (stuff_node[0]>=max_stuff_vis)
  {
    for (i=1;i<=max_stuff_vis;i++)
      printw("%d %s ",stuff[item[stuff_node[i]].num],item[stuff_node[i]].symbol);refresh();
  }
  attroff(COLOR_PAIR(1));
  //stop color(black,white)
  return 0;
}

int direction(int code)
{
  int k=0,num=0;
  int x=0,y=0,z=0;

  x=position.x+direct[code-1][0];
  y=position.y+direct[code-1][1];
  z=position.z+direct[code-1][2];
  num=0;
  for (k=1;k<=item_n;k++)
  if (check_border(x,y,z)==0)
    if (map[x][y][z]==item[k].num) num=k;
  if (check_border(x,y,z)==0)
    if ((item[num].cross==1)||(map[x][y][z]==0))
    {
      map[position.x][position.y][position.z]=pos_old;
      pos_old=map[x][y][z];
      position.x=position.x+direct[code-1][0];
      position.y=position.y+direct[code-1][1];
      position.z=position.z+direct[code-1][2];

      update_dstart();

      map[position.x][position.y][position.z]=1;
    }

  return 0;
}

int get_pro()
{
  int ch=0,r_num=0;
  halfdelay(1);
  ch=getch();
  //directions
  if (ch==KEY_LEFT) direction(1);
  if (ch==KEY_RIGHT) direction(2);
  if (ch==KEY_UP) direction(3);
  if (ch==KEY_DOWN) direction(4);
  if (ch=='.') direction(5);
  if (ch=='/') direction(6);

  //get stuff
  if (ch=='a') get_stuff(1);
  if (ch=='d') get_stuff(2);
  if (ch=='w') get_stuff(3);
  if (ch=='s') get_stuff(4);
  if (ch=='j') get_stuff(5);
  if (ch=='k') get_stuff(6);

  //right touch the stuff
  if ((ch==1)&&(position.y-1>=min_width)) right_touch(position.x,position.y-1,position.z);
  if ((ch==4)&&(position.y+1<=max_width)) right_touch(position.x,position.y+1,position.z);
  if ((ch==23)&&(position.x-1>=min_height)) right_touch(position.x-1,position.y,position.z);
  if ((ch==19)&&(position.x+1<=max_height)) right_touch(position.x+1,position.y,position.z);
  if ((ch==10)&&(position.z-1>=min_deep)) right_touch(position.x,position.y,position.z-1);
  if ((ch==11)&&(position.z+1<=max_deep)) right_touch(position.x,position.y,position.z+1);

  //change the stuff_now
  if ((ch=='0')&&(stuff_node[0]>=0)&&(0<=max_stuff_vis)) stuff_now=0;
  if ((ch=='1')&&(stuff_node[0]>=1)&&(1<=max_stuff_vis)) stuff_now=1;
  if ((ch=='2')&&(stuff_node[0]>=2)&&(2<=max_stuff_vis)) stuff_now=2;
  if ((ch=='3')&&(stuff_node[0]>=3)&&(3<=max_stuff_vis)) stuff_now=3;
  if ((ch=='4')&&(stuff_node[0]>=4)&&(4<=max_stuff_vis)) stuff_now=4;
  if ((ch=='5')&&(stuff_node[0]>=5)&&(5<=max_stuff_vis)) stuff_now=5;
  if ((ch=='6')&&(stuff_node[0]>=6)&&(6<=max_stuff_vis)) stuff_now=6;
  if ((ch=='7')&&(stuff_node[0]>=7)&&(7<=max_stuff_vis)) stuff_now=7;
  if ((ch=='8')&&(stuff_node[0]>=8)&&(8<=max_stuff_vis)) stuff_now=8;
  if ((ch=='9')&&(stuff_node[0]>=9)&&(9<=max_stuff_vis)) stuff_now=9;

  //Open and Save the saves
  if (ch==15)
  {
    r_num=read_save();
    if (r_num==1) return 2;
  }
  if (ch==6) write_save();

  //Change display_mode
  if (ch=='\t')
  {
    display_mode=display_mode+1;
    if (display_mode==4) display_mode=1;
  }

  if (ch==27) return 1;
  return 0;
}

int main_loop()
{
  int r=0;
  display();
  while (1)
  {
    r=0;
    r=get_pro();
    if (r>0) break;
    get_row_col();
    display();
  }
  return 0;
}

int init_all()
{
  init_license();
  init_digital();
  return 0;
}

int init_license()
{
  setlocale(LC_ALL, "en_US.UTF-8");
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);
  if (has_colors()==TRUE) start_color();
  printw("Welcome to Craft-L!\n");
  printw("The software is using GPL (http://www.gnu.org/licenses/gpl.txt)\n");
  printw("Press any key to agree and play\n");
  refresh();
  getch();
  return 0;
}

int init_digital()
{
  int i=0;
  item_init();
  display_mode=1;

  if (horizon>=200) horizon--;
  if (horizon<=0) horizon++;

  position.x=(abs(GetRandom())%(max_height-100)+1);
  position.y=(abs(GetRandom())%(max_width-100)+1);
  position.z=horizon;

  pos_old=map[position.x][position.y][position.z];
  get_row_col();

  dstart_x=position.x;
  dstart_y=position.y;
  dstart_z=position.z;
  dstart_x_deep=position.x;
  check_dstart_border();

  map[position.x][position.y][position.z]=1;
  for (i=1;i<=item_n;i++)
    if (item[i].use==0)
      random_stuff(item[i].num,item[i].chance,item[i].fluc,item[i].liquid);
  for (i=1;i<=item_n;i++)
    if (item[i].use==1)
      server_landform(i);
  return 0;
}

void cleanup()
{
  endwin();
}

int main()
{
  atexit(cleanup);
  init_all();
  main_loop();
  return 0;
}
