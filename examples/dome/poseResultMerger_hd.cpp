#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

int main(int argc, char** argv)
{
    printf("PoseResultMerger_hd\n");
    if (argc != 6)
    {
        printf("Usage: poseResultMerger.sh inputFolder outputFolder frameStart frameEnd numberBodyParts\n");
        return -1;
    }
    const char* input_folder = argv[1];//"/media/posefs1b/Users/hanbyulj";
    const char* output_folder = argv[2];//"/media/posefs1b/Users/hanbyulj/poseDetect_pm";

    mkdir(output_folder,755);
    int startIdx = atoi(argv[3]);//1000;
    int endIdx = atoi(argv[4]);//1000;
    int numberBodyParts = atoi(argv[5]);//1000;


    printf("## inputFolder: %s\n",input_folder);
    printf("## oututFolder: %s\n",output_folder);
    printf("## frames: %d -> %d\n",startIdx,endIdx);
    printf("## numberBodyParts: %d\n",numberBodyParts);



    for(int f=startIdx;f<=endIdx;++f)
    {
        char outputFile[512];
        sprintf(outputFile,"%s/poseDetectMC_hd%08d.txt",output_folder,f);
        float buf[100*3*25];      //maximum 100 people
        ofstream fout(outputFile);
        fout << "ver 0.5\n";

        // Count valid views
        int viewCnt=0;
        int p=0; // for(int p=1;p<=20;++p)
        {
            for(int c=0;c<=30;++c)
            {
                char filename[512];

                sprintf(filename,"%s/%03dXX/%08d/hd%08d_%02d_%02d.txt",input_folder,int(f/100),f,f,p,c);
                ifstream fin(filename, ios::binary);
                if(fin.is_open()==false)
                    continue;
                viewCnt++;
                fin.close();
            }
        }

        printf("Frame: %d :: numViews: %d\n",f,viewCnt);
        fout << "processedViews " <<viewCnt<<"\n";
        const int personArea = 3*numberBodyParts;
        std::cout << "peopleNum: ";
        p=0; // for(int p=0;p<=0;++p)
        {
            for(int c=0;c<=30;++c)
            {
                char filename[512];

                sprintf(filename,"%s/%03dXX/%08d/hd%08d_%02d_%02d.txt",input_folder,int(f/100),f,f,p,c);
                //printf("%s\n",filename);
                ifstream fin(filename, ios::binary);
                if(fin.is_open()==false)
                    continue;

                // printf("fileName: %s\n",filename);
                int peopleNum;//,memoryVolume;
                fin.read((char*)&peopleNum,sizeof(int));
                // printf("peopleNum: %d\n",peopleNum);
                std::cout << peopleNum << " ";
                const int memoryVolume = peopleNum*personArea;

                fin.read((char*)buf,sizeof(float)*memoryVolume);
                fout <<f <<" " << p << " " <<c << " "<< peopleNum << " " <<numberBodyParts <<"\n";
                for(int h=0;h<peopleNum;++h)
                {
                    for(int j=0;j<personArea;++j)
                        fout << buf[personArea*h+j] << " ";
                    fout << "\n";
                }
                fin.close();
            }
        }
        fout.close();
        std::cout << std::endl;
    }

/*
    folderName=A

    char saveFileName[512];
    sprintf(saveFileName,"%s/%s.txt",folderName.c_str(),filename.c_str());
    ofstream fout(saveFileName,ios::binary);
    fout.write((char*)&poseMemSize,sizeof(int));
    fout.write((char*)poseData,sizeof(float)*poseMemSize);
    delete[] poseData;
    fout.close();
*/
}
