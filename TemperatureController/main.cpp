//
//  main.cpp
//  TemperatureController
//
//  Created by Jignesh Panchal on 11/6/22.
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <string.h>
#include <algorithm>

#ifdef __APPLE__
// Defined before OpenGL and GLUT includes to avoid deprecation messages
#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#define GLEW_STATIC

//#include <glad/glad.h>
//GLEW
#include <GL/glew.h>
//GLFW
#include <GLFW/glfw3.h>
//GLUT
#include <GL/glut.h>
#include <GL/glx.h>
#endif

#define MINIMUM(a,b) (((a)<(b))?(a):(b))
#define MAXIMUM(a,b) (((a)>(b))?(a):(b))

void plotSystemRunParameters();
void initSystemRunScreen(void);
void save_plot(void);

#define ROOM_TEMPERATURE_C 25.0

#define NOPAT 1000000

#define NUM_TRAINING_SAMPLES 26

#define OUTNOD 1
#define INNOD 2
#define HINOD 3

typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define MAX 256

enum {
    MENU_RECTANGLE = 1, // 1
    MENU_BARTLETT,      // 2
    MENU_HANNING,       // 3
    MENU_HAMMING,       // 4
    MENU_BLACKMAN,      // 5
    MENU_FILTER,        // f
    MENU_LOWPASS,       // l
    MENU_HIGHPASS,      // h
    MENU_BANDPASS,      // b
    MENU_BANDSTOP,      // p
    MENU_INPUT,         // i
    MENU_SINE,          // s
    MENU_SQUARE,        // q
    MENU_NOISE,         // n
    MENU_NOISYSINE,     // ns
    MENU_NOISYSQUARE,   // nq
    MENU_OUTPUT,        // o
    MENU_EXIT,          // Esc
    MENU_EMPTY
};

static BOOL g_bButton1Down = FALSE;
static BOOL g_bButtonLEFTDown = FALSE;
static BOOL g_bButtonRIGHTDown = FALSE;
static BOOL g_bPause = FALSE;
static BOOL g_bTrain = FALSE;
static BOOL g_bSimu = FALSE;
static BOOL g_bReplay = FALSE;

static int g_yClick = 0;

static int g_Width = 640;                          // Initial window width
static int g_Height = 480;

int sp;
int skip;

int mainWindowX=1,smallWindowX=1;


char st[80] = "./NetData/FieldRun_replay_setTemp200c_0.txt";

char st_input_training_data[80] = "./NetData/NNTempContrl_TrainingData_v6.txt";

char st_in_training_vij[80] = "./NetData/NNTempContrl_vij_temp.bin";
//char st_vij[80] = "./NetData/NNTempContrl_vij_v6.bin";
char st_vij[80] = "./NetData/NNTempContrl_vij_temp.bin";

char st_in_training_wjk[80] = "./NetData/NNTempContrl_wjk_temp.bin";
//char st_wjk[80] = "./NetData/NNTempContrl_wjk_v6.bin";
char st_wjk[80] = "./NetData/NNTempContrl_wjk_temp.bin";

char ch;

int honc,color=14,xm=0,m=0,pdpoint=60,count;
float rate,vt=25,vpm=25,vx=25,setpoint=200,err_old=25;
float t1=25,t2=25;
float v[INNOD][HINOD],w[HINOD][OUTNOD];

float temp=ROOM_TEMPERATURE_C;
float temp_prev_measured=ROOM_TEMPERATURE_C;
int temp_prev=0;
int heatingOnCycle=0, coolingOnCycle=0;
int heatingOnCycle_prev=0,coolingOnCycle_prev=0;
int count_entry_into_simulateSystem=0;
static BOOL new_simu = TRUE;
static BOOL simu_end = FALSE;

static BOOL replay_end = FALSE;

FILE *finput;
float training_input[INNOD],desired_output[OUTNOD];
float delw[HINOD][OUTNOD],delv[INNOD][HINOD];
float b[HINOD];
float del3[OUTNOD],del2[HINOD],del[HINOD];
float eta=0.1,alp=0.1,error=0.0,err=0.0;
float eta_prev=0.1, alp_prev=0.1, error_prev=0.0, err_prev=0.0;
static BOOL new_training = TRUE;
static BOOL training_end = FALSE;

float sigfn(float input)
{
    return (float)(1/(1+exp(-input)));
}

float partde(float input)
{
    return (float)( exp(-input) / ((1+exp(-input)) * (1+exp(-input))) );
}


void simulate_heater_blower(int duty_cycle,int mode)
{
    int j=0;

    switch(mode){
    case 0    :for(j=0;j<duty_cycle;j++){
            t1+=0.05*exp(j*0.25);
            t2+=(t1 - temp)/t1;
            temp = t2;
        }
        for(j=duty_cycle;j<10;j++){
            t1-=0.0001*exp(j*0.001);
            t2+=(t1 - temp)/t1;
            temp = t2;
        }
        break;
    case 1    :for(j=0;j<duty_cycle;j++){
            t1-=0.055*exp(j*0.25);
            t2-=(t1 - temp)/t1;
            temp = t2;
        }
        for(j=duty_cycle;j<10;j++){
            t1-=0.0001*exp(j*0.001);
            t2+=(t1 - temp)/t1;
            temp = t2;
        }
        break;
    }
        
    if(temp<10)temp=10;
    printf("\n %3.2f",temp);
    //getch();
}

int XcordTransformBGIToGLUT(int x, int xmax_glut)
{
    return (x - (xmax_glut/2));
}

int YcordTransformBGIToGLUT(int y, int ymax_glut)
{
    return (-y + (ymax_glut/2));
}

int XcordTransformGLUTToBGI(int x_glut, int xmax_glut)
{
    return (x_glut + (xmax_glut/2));
}

int YcordTransformGLUTToBGI(int y_glut, int ymax_glut)
{
    return (-y_glut + (ymax_glut/2));
}

/**
* Draw a character string.
*
* @param x        The x position
* @param y        The y position
* @param string   The character string
*/
void drawString(int x, int y, char *string)
{
    glRasterPos2f(x, y);

    for (char* c = string; *c != '\0'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);  // Updates the position
    }
}

void drawBannerString(int x, int y, char *string)
{
    glRasterPos2f(x, y);

    for (char* c = string; *c != '\0'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);  // Updates the position
    }
}

void drawMidBannerString(int x, int y, char *string)
{
    glRasterPos2f(x, y);

    for (char* c = string; *c != '\0'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);  // Updates the position
    }
}

void initSystemDisplayScreen(void)
{
    int i;
    char tc_string[100];
    
    //----------------------
    glColor3f (1.0, 1.0, 0.5);
    glLineWidth(2.0);
    glBegin (GL_LINE_LOOP);
        glVertex2d( XcordTransformBGIToGLUT(214,g_Width), YcordTransformBGIToGLUT(0,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(639,g_Width), YcordTransformBGIToGLUT(0,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(639,g_Width), YcordTransformBGIToGLUT(70,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(214,g_Width), YcordTransformBGIToGLUT(70,g_Height) );
    glEnd();
    glColor3f (1.0, 1.0, 1.0);
    snprintf(tc_string, 100, "Industrial Process Temperature Controller");
    drawBannerString(XcordTransformBGIToGLUT(214+30,g_Width),YcordTransformBGIToGLUT(20,g_Height),tc_string);
    snprintf(tc_string, 100, "Based On");
    drawMidBannerString(XcordTransformBGIToGLUT(214+160,g_Width),YcordTransformBGIToGLUT(40,g_Height),tc_string);
    snprintf(tc_string, 100, "Neural Network : 3-layer Back Propagation Supervised Learning");
    drawMidBannerString(XcordTransformBGIToGLUT(214+20,g_Width),YcordTransformBGIToGLUT(60,g_Height),tc_string);
    //----------------------
    
    //---- Clear out the MODE Area -----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(10,g_Width),YcordTransformBGIToGLUT(10,g_Height),XcordTransformBGIToGLUT(214-5,g_Width),YcordTransformBGIToGLUT(70-5,g_Height));
    
    glColor3f (1.0, 1.0, 0.5);
    glLineWidth(2.0);
    glBegin (GL_LINE_LOOP);
        glVertex2d( XcordTransformBGIToGLUT(0,g_Width), YcordTransformBGIToGLUT(0,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(214,g_Width), YcordTransformBGIToGLUT(0,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(214,g_Width), YcordTransformBGIToGLUT(70,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(0,g_Width), YcordTransformBGIToGLUT(70,g_Height) );
    glEnd();
    glColor3f (1.0, 1.0, 1.0);
    snprintf(tc_string, 100, "Press t to Train");
    drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(25,g_Height),tc_string);
    snprintf(tc_string, 100, "Press s to Simulate");
    drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(40,g_Height),tc_string);
    snprintf(tc_string, 100, "Press r to Replay");
    drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(55,g_Height),tc_string);
    //----------------------
    
    //-----------MAIN Window --------
    glColor3f (1.0, 1.0, 0.5);
    glLineWidth(2.0);
    glBegin (GL_LINE_LOOP);
        glVertex2d( XcordTransformBGIToGLUT(0,g_Width), YcordTransformBGIToGLUT(70,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(425,g_Width), YcordTransformBGIToGLUT(70,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(425,g_Width), YcordTransformBGIToGLUT(349,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(0,g_Width), YcordTransformBGIToGLUT(349,g_Height) );
    glEnd();
    //-------GRID for Main Plotting Window-----
    glColor3f (1.0, 1.0, 1.0);
    glLineWidth(0.1);
    glLineStipple(1, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINES);
        for(i=0;i<=260;i+=20)
        {
            glVertex2d(XcordTransformBGIToGLUT(0,g_Width),YcordTransformBGIToGLUT(349-i,g_Height));
            glVertex2d(XcordTransformBGIToGLUT(425,g_Width),YcordTransformBGIToGLUT(349-i,g_Height));
        }
    glEnd();
    glBegin(GL_LINES);
        for(i=25;i<=425;i+=25)
        {
            glVertex2d(XcordTransformBGIToGLUT(i,g_Width),YcordTransformBGIToGLUT(20+70,g_Height));
            glVertex2d(XcordTransformBGIToGLUT(i,g_Width),YcordTransformBGIToGLUT(349,g_Height));
        }
    glEnd();
    //----------------------
    
    
    //----------Top Small Window -----------
    glColor3f (1.0, 1.0, 0.5);
    glLineWidth(2.0);
    glDisable(GL_LINE_STIPPLE);
    glBegin (GL_LINE_LOOP);
        glVertex2d( XcordTransformBGIToGLUT(425,g_Width), YcordTransformBGIToGLUT(70,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(639,g_Width), YcordTransformBGIToGLUT(70,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(639,g_Width), YcordTransformBGIToGLUT(210,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(425,g_Width), YcordTransformBGIToGLUT(210,g_Height) );
    glEnd();
    //-------GRID for Top Small Window -----
    glColor3f (1.0, 1.0, 1.0);
    glLineWidth(0.1);
    glLineStipple(1, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINES);
        for(i=0;i<=120;i+=10)
        {
            glVertex2d(XcordTransformBGIToGLUT(425,g_Width),YcordTransformBGIToGLUT(210-i,g_Height));
            glVertex2d(XcordTransformBGIToGLUT(639,g_Width),YcordTransformBGIToGLUT(210-i,g_Height));
        }
    glEnd();
    glBegin(GL_LINES);
        for(i=12;i<=214;i+=12)
        {
            glVertex2d(XcordTransformBGIToGLUT(425+i,g_Width),YcordTransformBGIToGLUT(70+20,g_Height));
            glVertex2d(XcordTransformBGIToGLUT(425+i,g_Width),YcordTransformBGIToGLUT(210,g_Height));
        }
    glEnd();
    //-----------------------
    
    //-------Bottom Small Window --------------
    glColor3f (1.0, 1.0, 0.5);
    glLineWidth(2.0);
    glDisable(GL_LINE_STIPPLE);
    glBegin (GL_LINE_LOOP);
        glVertex2d( XcordTransformBGIToGLUT(425,g_Width), YcordTransformBGIToGLUT(210,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(639,g_Width), YcordTransformBGIToGLUT(210,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(639,g_Width), YcordTransformBGIToGLUT(349,g_Height) );
        glVertex2d( XcordTransformBGIToGLUT(425,g_Width), YcordTransformBGIToGLUT(349,g_Height) );
    glEnd();
    //-------GRID for Bottom Small Window -----
    glColor3f (1.0, 1.0, 1.0);
    glLineWidth(0.1);
    glLineStipple(1, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINES);
        for(i=0;i<=120;i+=10)
        {
            glVertex2d(XcordTransformBGIToGLUT(425,g_Width),YcordTransformBGIToGLUT(349-i,g_Height));
            glVertex2d(XcordTransformBGIToGLUT(639,g_Width),YcordTransformBGIToGLUT(349-i,g_Height));
        }
    glEnd();
    glBegin(GL_LINES);
        for(i=12;i<=214;i+=12)
        {
            glVertex2d(XcordTransformBGIToGLUT(425+i,g_Width),YcordTransformBGIToGLUT(210+20,g_Height));
            glVertex2d(XcordTransformBGIToGLUT(425+i,g_Width),YcordTransformBGIToGLUT(349,g_Height));
        }
    glEnd();
    //-----------------------
}

void displayError(void)
// Update Error Reading window
{
    char plot_string[100];
    
    //---- Clear out the area-----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(125,g_Width),YcordTransformBGIToGLUT(70+15,g_Height),XcordTransformBGIToGLUT(125+200,g_Width),YcordTransformBGIToGLUT(70+5,g_Height));
    glColor3f (1.0, 1.0, 1.0);
    snprintf(plot_string, 100, "Error : %.3f",error);
    drawString(XcordTransformBGIToGLUT(125,g_Width),YcordTransformBGIToGLUT(15+70,g_Height),plot_string);
    
    glColor3f (1.0, 1.0, 1.0);
    //glPointSize(2.0f);
    //glBegin(GL_POINTS);
    glLineWidth(2.0);

    glBegin(GL_LINE_LOOP);
        glVertex2d(XcordTransformBGIToGLUT(mainWindowX-1,g_Width),YcordTransformBGIToGLUT(349-fmin(275,(int)(error_prev*10)),g_Height));
        glVertex2d(XcordTransformBGIToGLUT(mainWindowX,g_Width),YcordTransformBGIToGLUT(349-fmin(275,(int)(error*10)),g_Height));
    glEnd();
    //---- Clear out the area-----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(10,g_Width),YcordTransformBGIToGLUT(10,g_Height),XcordTransformBGIToGLUT(214-5,g_Width),YcordTransformBGIToGLUT(70-5,g_Height));
    //----------------------------
    glColor3f (0.0, 1.0, 0.0);
    snprintf(plot_string, 100, "Training In Progress");
    drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(45,g_Height),plot_string);
    //--------------------
}


void displayEta(void)
{
    char plot_string[100];
    
    //---- Clear out the area-----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(425+30,g_Width),YcordTransformBGIToGLUT(70+15,g_Height),XcordTransformBGIToGLUT(425+200,g_Width),YcordTransformBGIToGLUT(70+5,g_Height));
    glColor3f (1.0, 1.0, 1.0);
    snprintf(plot_string, 100, " Eta : %.3f",eta);
    drawString(XcordTransformBGIToGLUT(425+30,g_Width),YcordTransformBGIToGLUT(70+15,g_Height),plot_string);
    //----- Heater ---------
    glColor3f (1.0, 0.0, 0.0);
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
        glVertex2d(XcordTransformBGIToGLUT(425+smallWindowX-1,g_Width),YcordTransformBGIToGLUT(210-10-fmin(100,(int)(100*eta_prev)),g_Height));
        glVertex2d(XcordTransformBGIToGLUT(425+smallWindowX,g_Width),YcordTransformBGIToGLUT(210-10-fmin(100,(int)(100*eta)),g_Height));
    glEnd();
    //---------------------
}

void displayAlpha(void)
{
    char plot_string[100];
    
    //---- Clear out the area-----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(425+30,g_Width),YcordTransformBGIToGLUT(210+15,g_Height),XcordTransformBGIToGLUT(425+200,g_Width),YcordTransformBGIToGLUT(210+5,g_Height));
    glColor3f (1.0, 1.0, 1.0);
    snprintf(plot_string, 100, " Alpha : %.3f", alp);
    drawString(XcordTransformBGIToGLUT(425+30,g_Width),YcordTransformBGIToGLUT(210+15,g_Height),plot_string);
    
    //----- Blower ---------
    glColor3f (0.0, 0.0, 1.0);
    //glRasterPos2f(XcordTransformBGIToGLUT(425+k-1,g_Width),YcordTransformBGIToGLUT(349-10-(10*oldbpw),g_Height));
    //glPointSize(2.0f);
    //glBegin(GL_POINTS);
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
        glVertex2d(XcordTransformBGIToGLUT(425+smallWindowX-1,g_Width),YcordTransformBGIToGLUT(349-10-fmin(100,(int)(100*alp_prev)),g_Height));
        glVertex2d(XcordTransformBGIToGLUT(425+smallWindowX,g_Width),YcordTransformBGIToGLUT(349-10-fmin(100,(int)(100*alp)),g_Height));
    glEnd();
    //---------------------
}

void plotSystemTrainingParameters(void)
{
    //------- Refresh the plot -------
    if(!(mainWindowX%2))smallWindowX++;mainWindowX++;
    if(mainWindowX>=425)
    {
        //Clear information from last draw
        glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
        
        //Switch to the drawing perspective
        glMatrixMode(GL_MODELVIEW);
        
        //Start with identity matrix
        glLoadIdentity();
        
        initSystemDisplayScreen();
        
        mainWindowX=1;
        smallWindowX=1;
    }
    //--------------------------------
    
    // Update measurements/readings
    displayError();
    displayEta();
    displayAlpha();
    
    //-------------------------------
    // Update old values
    //-------------------------------
    eta_prev = eta;
    alp_prev = alp;
    error_prev = error;
    //-------------------------------
    
    glFlush();
}

void InitializeNeuralNetwork(void)
{
    int i, j, k;
        
    for(i=0;i<INNOD;i++)
    {
        for(j=0;j<HINOD;j++)
        {
            v[i][j]=(0.5-(float)((rand()%1001)/1000.0))*15.0;
            delv[i][j]=0;
        }
    }
    for(j=0;j<HINOD;j++)
    {
        for(k=0;k<OUTNOD;k++)
        {
            w[j][k]=(0.5-(float)((rand()%1001)/1000.0))*15.0;
            delw[j][k]=0;
        }
    }
}

void SaveTrainedNeuralNetwork(void)
{
    int i,j,k;
    FILE *fv, *fw;
    
    //---------------------------------------------------------------------
    //Save Vij weights
    //---------------------------------------------------------------------
    if((fv=fopen(st_in_training_vij,"wb"))==NULL)
    {
        printf("Error : Can't open training_vij.data file \n");
        exit(0);
    }
    for(i=0;i<INNOD;i++)
    {
        for(j=0;j<HINOD;j++)
        {
            fwrite(&v[i][j],sizeof(float),1,fv);
        }
    }
    fclose(fv);
    
    //---------------------------------------------------------------------
    //Save Wjk weights
    //---------------------------------------------------------------------
    if((fw=fopen(st_in_training_wjk,"wb"))==NULL)
    {
        printf("Error : Can't open training_wjk.data file \n");
        exit(0);
    }

    for(j=0;j<HINOD;j++)
    {
        for(k=0;k<OUTNOD;k++)
        {
            fwrite(&w[j][k],sizeof(float),1,fw);
        }
    }
    fclose(fw);
}

void OpenInputTrainingVectorFile(void)
{
    //---------------------------------------------------------------------
    // Open Training Data file
    // File should contains NUM_TRAINING_SAMPLES = 25
    //---------------------------------------------------------------------
    if((finput=fopen(st_input_training_data,"rt"))==NULL)
    {
        printf("Error : Can't open training_input_data file \n");
        exit(0);
    }
}

void get_new_data(void)
{
    int i,k;

    for(i=0;i<INNOD;i++)
    {
        fscanf(finput,"%f",&training_input[i]);
    }
    for(k=0;k<OUTNOD;k++)
    {
        fscanf(finput,"%f",&desired_output[k]);
    }
}

void TrainNeuralNetwork(void)
{
    float err=0.0;
    float ct;
    int i,j,k,l,m,ch;
    
    error = 0;
    
    // Start from
    rewind(finput);
    
    for(l=0;l<NUM_TRAINING_SAMPLES;l++)
    {
        //Get new data line for the training
        get_new_data();
        
        for(k=0;k<OUTNOD;k++)
        {
            del3[k] = 0.0;
            ct=0.0;
        
            for(j=0;j<HINOD;j++)
            {
                b[j]=0.0;
                for(i=0;i<INNOD;i++)
                {
                    b[j]=v[i][j]*(training_input[i])+b[j];
                }
                b[j]=sigfn(b[j]);
                ct=w[j][k]*b[j]+ct;
            }
            del3[k] = partde(ct);
            ct=sigfn(ct);
            err=desired_output[k] - ct;
            
            /*printf("\n%f : %f\n",desired_output[k],ct);
             if(fabs(err)<0.05)err=0.0;*/
            error =  fabs(err) + error;
            del3[k] =  err * del3[k];
        }
        
        for(j=0;j<HINOD;j++)
        {
            del[j] = 0;
            for(k=0;k<OUTNOD;k++)
            {
                del[j] = del3[k]*w[j][k]+del[j];
            }
        }
        
        
        for(j=0;j<HINOD;j++)
        {
            del2[j]=0;
            for(i=0;i<INNOD;i++)
            {
                del2[j]=v[i][j]*training_input[i]+del2[j];
            }
            del2[j] = del[j] * partde(del2[j]);
        }
        
        for(i=0;i<INNOD;i++)
        {
            for(j=0;j<HINOD;j++)
            {
                delv[i][j] = (eta*del2[j]*training_input[i]) +(alp*delv[i][j]);
                v[i][j] = v[i][j] + delv[i][j];
            }
        }
        
        for(j=0;j<HINOD;j++)
        {
            for(k=0;k<OUTNOD;k++)
            {
                delw[j][k] = (eta * del3[k] *b[j])  +  (alp * delw[j][k]);
                w[j][k] = w[j][k] + delw[j][k];
            }
        }
        
    }
    //printf("\nerror =   %f   ",error);
    plotSystemTrainingParameters();
}


void displayTemperature(void)
// Update Temperate Reading window
{
    char plot_string[100];
    
    
    //---- Clear out the area-----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(125,g_Width),YcordTransformBGIToGLUT(70+15,g_Height),XcordTransformBGIToGLUT(125+200,g_Width),YcordTransformBGIToGLUT(70+5,g_Height));
    glColor3f (1.0, 1.0, 1.0);
    snprintf(plot_string, 100, "Measured Temperature  : %d 'C",(int)temp);
    drawString(XcordTransformBGIToGLUT(125,g_Width),YcordTransformBGIToGLUT(15+70,g_Height),plot_string);
    
    
    //----- Temp --------
    glColor3f (1.0, 1.0, 1.0);
    //glPointSize(2.0f);
    //glBegin(GL_POINTS);
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
        glVertex2d(XcordTransformBGIToGLUT(mainWindowX-1,g_Width),YcordTransformBGIToGLUT(349-fmin(275,temp_prev),g_Height));
        glVertex2d(XcordTransformBGIToGLUT(mainWindowX,g_Width),YcordTransformBGIToGLUT(349-fmin(275,temp),g_Height));
    glEnd();
    
    
    //---- Clear out the area-----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(10,g_Width),YcordTransformBGIToGLUT(10,g_Height),XcordTransformBGIToGLUT(214-5,g_Width),YcordTransformBGIToGLUT(70-5,g_Height));
    glColor3f (1.0, 1.0, 1.0);
    
    if(g_bSimu)
    {
        snprintf(plot_string, 100, "Set Temperature            : %d 'C",(int)setpoint);
        drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(30,g_Height),plot_string);
        glColor3f (0.0, 1.0, 0.0);
        snprintf(plot_string, 100, "Simulation In Progress");
        drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(45,g_Height),plot_string);
        
    }
    if(g_bReplay)
    {
        glColor3f (0.0, 1.0, 0.0);
        snprintf(plot_string, 100, "Replay In Progress");
        drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(45,g_Height),plot_string);
    }
}


void displayHeatingDutyCycles(void)
{
    char plot_string[100];
    
    //---- Clear out the area-----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(425+30,g_Width),YcordTransformBGIToGLUT(70+15,g_Height),XcordTransformBGIToGLUT(425+200,g_Width),YcordTransformBGIToGLUT(70+5,g_Height));
    glColor3f (1.0, 1.0, 1.0);
    snprintf(plot_string, 100, " Heating ON Cycle : %d%%",(heatingOnCycle*10));
    drawString(XcordTransformBGIToGLUT(425+30,g_Width),YcordTransformBGIToGLUT(70+15,g_Height),plot_string);
    //----- Heater ---------
    glColor3f (1.0, 0.0, 0.0);
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
        glVertex2d(XcordTransformBGIToGLUT(425+smallWindowX-1,g_Width),YcordTransformBGIToGLUT(210-10-fmin(100,(10*heatingOnCycle_prev)),g_Height));
        glVertex2d(XcordTransformBGIToGLUT(425+smallWindowX,g_Width),YcordTransformBGIToGLUT(210-10-fmin(100,(10*heatingOnCycle)),g_Height));
    glEnd();
    //---------------------
}

void displayCoolingDutyCycles(void)
{
    char plot_string[100];
    
    //---- Clear out the area-----
    glColor3f (0.0, 0.0, 0.0);
    glRecti(XcordTransformBGIToGLUT(425+30,g_Width),YcordTransformBGIToGLUT(210+15,g_Height),XcordTransformBGIToGLUT(425+200,g_Width),YcordTransformBGIToGLUT(210+5,g_Height));
    glColor3f (1.0, 1.0, 1.0);
    snprintf(plot_string, 100, " Cooling ON Cycle : %d%%", (coolingOnCycle*10));
    drawString(XcordTransformBGIToGLUT(425+30,g_Width),YcordTransformBGIToGLUT(210+15,g_Height),plot_string);
    
    //----- Blower ---------
    glColor3f (0.0, 0.0, 1.0);
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
        glVertex2d(XcordTransformBGIToGLUT(425+smallWindowX-1,g_Width),YcordTransformBGIToGLUT(349-10-fmin(100,(10*coolingOnCycle_prev)),g_Height));
        glVertex2d(XcordTransformBGIToGLUT(425+smallWindowX,g_Width),YcordTransformBGIToGLUT(349-10-fmin(100,(10*coolingOnCycle)),g_Height));
    glEnd();
    //---------------------
}

void plotSystemRunParameters(void)
{
    //------- Refresh the plot -------
    if(!(mainWindowX%2))smallWindowX++;mainWindowX++;
    if(mainWindowX>=425)
    {
        //Clear information from last draw
        glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
        
        //Switch to the drawing perspective
        glMatrixMode(GL_MODELVIEW);
        
        //Start with identity matrix
        glLoadIdentity();
        
        initSystemDisplayScreen();
        //initSystemRunScreen();
        
        mainWindowX=1;
        smallWindowX=1;
    }
    //--------------------------------
    
    // Update measurements/readings
    displayTemperature();
    displayHeatingDutyCycles();
    displayCoolingDutyCycles();
    
    //-------------------------------
    // Update old values
    //-------------------------------
    temp_prev=temp;
    heatingOnCycle_prev=heatingOnCycle;
    coolingOnCycle_prev=coolingOnCycle;
    //-------------------------------
    
    glFlush();
}

void loadTrainedNeuralNetwork(void)
{
    int i,j,k;
    FILE *fw,*fv;

    //---------------------------------------------------------------------
    //Open Vij weights
    //---------------------------------------------------------------------
    if((fv=fopen(st_vij,"rb"))==NULL)
    {
        printf("Error : Can't open Vij.data file \n");
        exit(0);
    }
    for(i=0;i<INNOD;i++)
    {
        for(j=0;j<HINOD;j++)
        {
            fread(&v[i][j],sizeof(float),1,fv);
            //printf("\nv[%d][%d] = %4.2f",i,j,v[i][j]);
        }
    }
    fclose(fv);
    
    //---------------------------------------------------------------------
    //Open Wjk weights
    //---------------------------------------------------------------------
    if((fw=fopen(st_wjk,"rb"))==NULL)
    {
        printf("Error : Can't open Wjk.data file \n");
        exit(0);
    }

    for(j=0;j<HINOD;j++)
    {
        for(k=0;k<OUTNOD;k++)
        {
            fread(&w[j][k],sizeof(float),1,fw);
            //printf("\nw[%d][%d] = %4.2f",j,k,w[j][k]);
        }
    }
    fclose(fw);
}


void AdjustHeatingOnCycle_NeuralNetworkOutput(void)
{
    float xx[INNOD];
    float c,ct;
    int i,j;
    float pererror,error,cherr;

    error = setpoint - temp;
    
    if(error<-2)
    {
        heatingOnCycle=0;
        goto END;
    }
    
    if(error>pdpoint)
    {
        heatingOnCycle=10;
    }
    else
    {
        pererror = error/pdpoint;
        xx[0] =pererror;
        cherr = (err_old-temp)/4.0;
        if(!error)
        {
            if(cherr==-0.5)xx[1]=1.0;
            if(cherr==-1.0)xx[1]=2.0;
            if(cherr>=0)xx[1]=0.0;
        }
        else
        {
            if((cherr==-0.5)||(cherr==0))xx[1]=1.0;
            if(cherr==-1.0)xx[1]=2.0;
            if(cherr>0)xx[1]=0.0;
        }
        
        //----------NN---------------
        c=0;
        for(j=0;j<HINOD;j++)
        {
            ct=0;
            for(i=0;i<INNOD;i++)
            {
                ct=v[i][j]*(xx[i])+ ct;
            }
            c=w[j][0]*sigfn(ct) + c;
        }
        heatingOnCycle = (int)ceil(sigfn(c)*10.0);
        //--------------------------
    }
    
END:
    err_old=temp;
}

void temp_simu(void)
{
    //Update Temperature
    if(rate>0)honc++;
    vt = vt + 0.01*rate*exp(honc*1.5);
    temp += ((vt - temp_prev_measured)/vt);
    temp_prev_measured = temp;
}

void trainSystem(void)
{
    char plot_string[100];
    
    if(training_end)
    {
        fclose(finput);//closeInputTrainingVectorFile
        SaveTrainedNeuralNetwork();
        
        glColor3f(1.0,0.0,0.0);
        snprintf(plot_string, 100, "*** Training Ended ***\n");
        drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(65,g_Height),plot_string);
        glFlush();
    }
    else
    {
        if(!g_bPause)
        {
            if(new_training)
            {
                InitializeNeuralNetwork();
                OpenInputTrainingVectorFile();
                new_training = FALSE;
            }
            else
            {
                TrainNeuralNetwork();
            }
        }
        else
        {
            // Update Error Reading window
            displayError();
            displayEta();
            displayAlpha();
            
            glColor3f(0.0,1.0,1.0);
            snprintf(plot_string, 100, "*** PAUSED ***\n");
            drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(65,g_Height),plot_string);
            glFlush();
        }
    }
    
}

void simulateSystem(void)
{
    int flag=0;
    char plot_string[100];
    
    if(simu_end)
    {
        glColor3f(1.0,0.0,0.0);
        snprintf(plot_string, 100, "*** Simulation Ended ***\n");
        drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(65,g_Height),plot_string);
        glFlush();
    }
    else
    {
        if(!g_bPause)
        {
            if(new_simu)
            {
                loadTrainedNeuralNetwork();
                new_simu = FALSE;
            }
            count_entry_into_simulateSystem++;
            count++;
            if(!flag)
            {
                honc=1;
                rate=0.30;
                flag=1;
            }
            
            if(count >= 19 * heatingOnCycle)
            {
                rate=-0.1;
                flag=0;
            }
            
            if(count>=50)
            {
                plotSystemRunParameters();
            }
            
            if(count>=182)
            {
                count=0;
                AdjustHeatingOnCycle_NeuralNetworkOutput();
                if(heatingOnCycle <= 0)
                {
                    coolingOnCycle = 5;
                }
                else
                {
                    coolingOnCycle = 0;
                }
                //simulate_heater_blower(heatingOnCycle,0);
                //simulate_heater_blower(coolingOnCycle,1);
            }
            
            //Update Temperature
            temp_simu();
        }
        else
        {
            // Update Temperate Reading window
            displayTemperature();
            displayCoolingDutyCycles();
            displayHeatingDutyCycles();
            
            glColor3f(0.0,1.0,1.0);
            snprintf(plot_string, 100, "*** PAUSED ***\n");
            drawString(XcordTransformBGIToGLUT(20,g_Width),YcordTransformBGIToGLUT(65,g_Height),plot_string);
            glFlush();
        }
    }
}

void replaySystem(void)
{
    FILE *fp;
    int heat,blow,tp,count,skip=0;

    //replay_end --- figure out how to deal with this
    
    //---------------------------------------------------------------------
    //Load Replay Data file
    //---------------------------------------------------------------------
    if((fp=fopen(st,"rt"))==NULL)
    {
        printf("Error : Can't open Replay.txt file \n");
        exit(0);
    }
    
    count=skip;

    while(!feof(fp))
    {
        fscanf(fp,"%d %d %d\n",&blow,&heat,&tp);
        
        temp = (float)tp;
        heatingOnCycle = (float)heat;
        coolingOnCycle = (float)blow;
        
        if(heatingOnCycle <= 0)
        {
            heatingOnCycle = 0;
            coolingOnCycle = 5;
        }
        else
        {
            coolingOnCycle = 0;
        }
        
        if(count>=skip)
        {
            plotSystemRunParameters();
            count=0;
        }
        count++;
    }
    fclose(fp);
    //---------------------------------------------------------------------
}


void displaySystem(void)
{
    //Clear information from last draw
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    
    //Switch to the drawing perspective
    glMatrixMode(GL_MODELVIEW);
    
    //Start with identity matrix
    glLoadIdentity();

    initSystemDisplayScreen();
    
    //glFlush();
}

void reshape(GLint width, GLint height)
{
    //Viewport: area within drawing are displayed
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    
    //Setup viewing projection
    glMatrixMode(GL_PROJECTION);
    
    //Start with identity matrix
    glLoadIdentity();
    
    gluOrtho2D(-g_Width/2,g_Width/2,-g_Height/2,g_Height/2);
    
    //Switch to the drawing perspective
    glMatrixMode(GL_MODELVIEW);
    
    initSystemDisplayScreen();
    
    glFlush();
    //glutPostRedisplay();
}

void Keyboard(unsigned char key, int x, int y)
{
    //printf("%c\n",key);
    
    switch (key)
    {
        case '+':
            if(g_bSimu)
            {
                setpoint += 1;
                if (setpoint < ROOM_TEMPERATURE_C) setpoint = ROOM_TEMPERATURE_C;
                //glutPostRedisplay();
                //printf("fc1 = %f, fc2=%f\n",fc1,fc2);
            }
            if(g_bTrain)
            {
                eta+=0.01;
            }
            break;
        case '_':
            if(g_bSimu)
            {
                setpoint -= 1;
                if (setpoint < ROOM_TEMPERATURE_C) setpoint = ROOM_TEMPERATURE_C;
                //glutPostRedisplay();
                //printf("fc1 = %f, fc2=%f\n",fc1,fc2);
            }
            if(g_bTrain)
            {
                eta-=0.01;
            }
            break;
        case 'm':
            if(g_bTrain)
            {
                alp+=0.01;
            }
            break;
        case 'n':
            if(g_bTrain)
            {
                alp-=0.01;
            }
            break;
        case 'p':
            g_bPause = !g_bPause;
            break;
        case 'e':
            if(g_bTrain)
            {
                //Close training
                //Save weights
                //Close all open files
                training_end = TRUE;
            }
            if(g_bSimu)
            {
                //Close all open files
                simu_end = TRUE;
            }
            if(g_bReplay)
            {
                //Close all open files
                replay_end = TRUE;
            }
            initSystemDisplayScreen();
            break;
        case 't':
            if(g_bSimu)
            {
                g_bSimu = FALSE;
                if(simu_end){new_simu = TRUE;simu_end = FALSE;}
                mainWindowX = 1;
                smallWindowX = 1;
            }
            if(g_bReplay)
            {
                g_bReplay = FALSE;
                replay_end = FALSE;
                mainWindowX = 1;
                smallWindowX = 1;
            }
            if((training_end)&&(g_bTrain))
            {
                new_training = TRUE;
                training_end = FALSE;
                mainWindowX = 1;
                smallWindowX = 1;
                displaySystem();
                glutIdleFunc(trainSystem);
            }
            if(!g_bTrain)
            {
                g_bTrain = TRUE;
                displaySystem();
                glutIdleFunc(trainSystem);
            }
            break;
        case 's':
            if(g_bTrain)
            {
                g_bTrain = FALSE;
                if(training_end){new_training = TRUE;training_end = FALSE;}
                mainWindowX = 1;
                smallWindowX = 1;
            }
            if(g_bReplay)
            {
                g_bReplay = FALSE;
                replay_end = FALSE;
                mainWindowX = 1;
                smallWindowX = 1;
            }
            if((simu_end)&&(g_bSimu))
            {
                new_simu = TRUE;
                simu_end = FALSE;
                mainWindowX = 1;
                smallWindowX = 1;
                displaySystem();
                glutIdleFunc(simulateSystem);
            }
            if(!g_bSimu)
            {
                g_bSimu = TRUE;
                displaySystem();
                glutIdleFunc(simulateSystem);
            }
            break;
        case 'r':
            if(g_bTrain)
            {
                g_bTrain = FALSE;
                if(training_end){new_training = TRUE;training_end = FALSE;}
                mainWindowX = 1;
                smallWindowX = 1;
            }
            if(g_bSimu)
            {
                g_bSimu = FALSE;
                if(simu_end){new_simu = TRUE;simu_end = FALSE;}
                mainWindowX = 1;
                smallWindowX = 1;
            }
            if(!g_bReplay)
            {
                g_bReplay = TRUE;
                displaySystem();
                glutIdleFunc(replaySystem);
            }
            break;
        case 27: // ESCAPE key
            glutDestroyWindow(glutGetWindow());
            exit(0);
            break;
    }
}

void MouseButton(int button, int state, int x, int y)
{
    // Respond to mouse button presses.
    // If button1 pressed, mark this state so we know in motion function.
    if (button == GLUT_LEFT_BUTTON)
    {
        g_bButton1Down = (state == GLUT_DOWN) ? TRUE : FALSE;
        g_yClick = y - setpoint;
    }
    //printf("%d %d \n",button,state);
}

void MouseMotion(int x, int y)
{
    // If button1 pressed, zoom in/out if mouse is moved up/down.
    if (g_bButton1Down)
    {
        setpoint = (y - g_yClick);
        if (setpoint < ROOM_TEMPERATURE_C) setpoint = ROOM_TEMPERATURE_C;
        
        glFlush();
        //glutPostRedisplay();
    }
}


int main(int argc, char **argv)
{
    // GLUT Window Initialization:
    glutInit (&argc, argv);
    
    glutInitDisplayMode ( GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    
    //Position of Window appears on the terminal screen
    glutInitWindowPosition(0,0);
    
    //Size of the Window
    glutInitWindowSize(g_Width,g_Height);
    
    glutCreateWindow ("Temperature Controller Demo : Train, Simulate a Run and Reply a field Run");
    
    glClearColor(0.0, 0.0, 0.0, 1.0);         // BLACK background

    // Register callbacks:
    glutDisplayFunc(displaySystem);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseMotion);
    
    // Turn the flow of control over to GLUT
    glutMainLoop ();
    
    glutDestroyWindow(glutGetWindow());
    exit(EXIT_SUCCESS);
}
