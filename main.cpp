//
//  main.cpp
//  analyze_MAZ
//
//  Created by Mariana on 3/13/19.
//  Copyright © 2019 Mariana. All rights reserved.
//

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <exception>
#include <memory>
#include <fstream>
#include <cstddef>
#include <string>
#include <vector>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <iterator>
#include <cstring>
#include <numeric>


#if defined(_WIN32)
#define listcmd "dir /b *.dat > datfiles.txt 2>nul"
#elif defined(__GNUC__)
#define listcmd "ls *.dat > datfiles.txt 2>/dev/null"
#else
#error Unsupported compiler.
#endif


using namespace std;
using std::vector;


vector<int>Smooth(vector<int>& raw){
    //lambda function to make the transformation smooth the data
    auto smooth_filter = [](int& x){
        const int* iter = &x;
        auto val = (iter[-3] + 2 * iter[-2] + 3 * iter[-1] + 3 * iter[0] + 3 * iter[1] + 2 * iter[2] + iter[3]) / 15;
        return val;
    };
    
    vector<int> smooth;
    
    //negate data
    std::transform(raw.cbegin(),raw.cend(),raw.begin(),std::negate<int>());
    
    //smooth vector .resize(raw.size())
    smooth.resize(raw.size());
    
    //copy(begin(raw), begin(raw) + 3, begin(smooth));
    copy(begin(raw), begin(raw) + 3, begin(smooth));
    
    //transform(begin(raw) + 3, end(raw) - 3, begin(smooth) + 3, smooth_filter);
    transform(begin(raw) + 3, end(raw) - 3, begin(smooth) + 3, smooth_filter);
    
    
    //copy(end(raw) - 4,  end(raw), end(smooth) - 4);
    copy(end(raw) - 3, end(raw), end(smooth) - 3);
    
    return smooth;
}


vector<int> findPulse(vector<int>& smooth, vector<int>& peaks, int vt){
    vector<int> pulses;
    
    int start, peak;
    for (int i = 0; i < smooth.size(); i++){
        //prevent out of range
        if (i + 2 < smooth.size()){
            int temp = smooth.at(i+2) - smooth.at(i);
            if (temp > vt){
               start = i;
                cout << "pulse started at "<< i << endl;
                pulses.push_back(i);
                for (int j = i + 1; j < smooth.size(); j++){
                    if (smooth[j] > smooth[j + 1]){
                        peak = j;
                        peaks.push_back(peak);
                        i = j;
                        j = smooth.size();
                    }
                }
            }
        }
    }
    return pulses;
}


void findPiggyback(vector<int>& pulses, vector<int>& peaks, vector<int> smooth, int pulse_delta, double drop_ratio, int below_drop_ratio){
    for (int i = 0; i < pulses.size(); i++){
        size_t count = 0;
        if ((i + 1) < pulses.size()){
            int temp = pulses.at(i+1) - pulses.at(i);
            if (temp <= pulse_delta){
                int distance = pulses[i + 1] - peaks[i];
                int height = smooth.at(peaks[i]);
                
                int difference = (drop_ratio * height);
                
                for (size_t j = 0; j < distance; j++){
                    if (smooth.at(peaks[i] + j) < difference){
                        count++;
                    }
                }
                
                if (count > below_drop_ratio){
                    //cout << "Piggy found at " << pulses[i] << endl;
                    pulses.erase(begin(pulses) + i);
                    i--;
                }
            }
        }
    }
    
}

vector<int> findArea(vector<int>& pulses, vector<int>& raw, int width){
    vector<int> areas;
    
    for (size_t i = 0; i < pulses.size(); i++){
        if ((i + 1) < pulses.size()){
            if ((pulses[i+1] - pulses[i]) < width){
                areas.push_back(accumulate(begin(raw) + pulses[i], begin(raw) + pulses[i+1], 0));
            }
        }
        else{
            areas.push_back(accumulate(begin(raw) + pulses[i], begin(raw) + pulses[i] + width, 0));
        }
    }
    return areas;
}


void printArray(vector<int>& data){
    for (size_t i = 0; i < data.size(); i++){
        cout << data[i] << " ";
    }
    cout << "\n";
}


int main(int argc, const char *argv[])
{
    ifstream infoFile; //to read in the file
    infoFile.open(argv[1]);
    
    int vt, pulse_delta, width, below_drop_ratio;
    double drop_ratio;
    char buffer[1024];
    string infoLine;
    
    
    //----------------------------Parsing Method-------------------------------------
    
    if (argc > 1)
    {
        while (getline(infoFile, infoLine))
        {
            size_t p = infoLine.find_first_not_of("\t");
            infoLine.erase(0, p);
            
            if (infoLine[0] != '#')
            {
                strcpy(buffer, infoLine.c_str());
                char* token = strtok(buffer, "=");
                
                if (token != NULL)
                {
                    if (strcmp(token, "vt") == 0)
                    {
                        token = strtok(NULL, "\r");
                        vt = atoi(token);
                        //cout << vt << endl;
                    }
                    else if (strcmp(token, "pulse_delta") == 0)
                    {
                        token = strtok(NULL, "\r");
                        pulse_delta = atoi(token);
                        //cout << pulse_delta << endl;
                    }
                    else if (strcmp(token, "width") == 0)
                    {
                        token = strtok(NULL, "\r");
                        width = atoi(token);
                        //cout << width << endl;
                    }
                    else if (strcmp(token, "drop_ratio") == 0)
                    {
                        token = strtok(NULL, "\r");
                        drop_ratio = stod(token);
                        //cout << drop_ratio << endl;
                    }
                    else if (strcmp(token, "below_drop_ratio") == 0)
                    {
                        token = strtok(NULL, "\r");
                        below_drop_ratio = atoi(token);
                        //cout << below_drop_ratio << endl;
                    }
                }
            }
            else
            {
                continue;
            }
        }
    }
    else
    {
        cout << "Pass in a .ini file as command line argument please" << endl;
    }
    
    
    //-------------------------------Calls to other Methods--------------------------------
    
    
    system(listcmd); //creates datfiles.txt that holds all of my .dat files in
    
    fstream dat("datfiles.txt");
    
    string datLine;
    
    //dataFile.open(argv[2]);
    
    
    while (!dat.eof())
    {
        fstream dataFile;
        string dataLine;
        vector<int> raw, smooth, pulses, peaks, areas;
        
        getline(dat, datLine);
        dataFile.open(datLine);
        
        if (dataFile.good()){
            
            while (dataFile >> dataLine) // while (!inFile.eof())
            {
                raw.push_back(stoi(dataLine));
            }
            
            smooth = Smooth(raw);
            pulses = findPulse(smooth, peaks, vt);
            
            findPiggyback(pulses, peaks, smooth, pulse_delta, drop_ratio, below_drop_ratio);
            
            areas = findArea(pulses, raw, width);
            
            //------------------------PRINT--------------------------
            cout << datLine << ": ";
            printArray(pulses);
            printArray(areas);
            cout << "\n\n";
        }
    }
    
    getchar();
}








// for(i = 0; pulse.siz()

//int difference = *(piggyHelper+1) - peak;
// auto =
// int count = count_if(bigining iterator, end iterator, some kinda of lambda([](int& x)) ) {
// return something?
//}

/*
 dataFile.open(datLine);
 
 vector<int> raw, smooth, pulses, areas;
 
 
 while (dataFile >> dataLine) // while (!inFile.eof())
 {
 raw.push_back(stoi(dataLine));
 }
 
 
 
 //auto smooth_filter = [](int &x) {
 //        const int* iter = &x;
 //        auto val = (what he gave us);
 //        return val;
 
 
 smooth = Smooth(raw);
 
 
 for (size_t i = 0; i < smooth.size(); i++)
 {
 cout << smooth[i] << endl;
 }
 */

/*
 for (size_t i = 0; i < pulses.size(); i++)
 {
 cout << pulses[i] << endl;
 }
 
 */


//look for piggy backers


//now find the area under the curve







//TRASH
/*system(listcmd);
 ifstream datFiles("datfiles.txt");
 // string data;
 string dataIniFile;
 vector<int>v;
 vector<string>smooth;
 
 if (datFiles.is_open()){
 string dataFileName;
 while(datFiles >>dataFileName){
 cout << dataFileName;
 
 }
 }*/

//open files





//    ifstream file2;
//    file2.open(argv[2]);
//    string data2;
//    if (argc > 1) {
//        while (file2 >> data2) {
//            v.push_back(stoi(data2));
//        }
//    }
//    else{
//        cout << "Failed open file" << endl;
//        return 0;
//    }


//.DAT
//    system(listcmd);
//    fstream dat("datfiles.txt");
//    fstream dataFile;
//    string datLine;
//    string dataLine;
//
//    getline(dat, datLine);
//    dataFile.open(datLine);

//    while(dataFile >> dataLine){
//        v.push_back(stoi(dataLine));
//    }



//vector<int>peaks;
//for(int i = 0; i < pulses.size()-1; i++){
//    int count = 0;
//    int temp = pulses.at(i+1) - pulses.at(i);
//    if(temp <= pulse_delta){
//        //find peak
//        auto peak = max_element(begin(smooth) + pulses[i], begin(smooth) + pulses[i+1]);
//        int peakIdx = distance(begin(smooth), peak);
//        peaks.push_back(peakIdx); //
//        for(int j = peakIdx + 1; j < pulses[i+1]; j++){
//            if(smooth[j] < smooth[peakIdx] * drop_ratio){
//                //count++;
//                int diff = (drop_ratio * smooth[peakIdx] - smooth[j]);
//                if(diff > below_drop_ratio){
//                    cout << "found piggyback at " << pulses[i] << endl;
//                    pulses.erase(pulses.begin()+1);
//                    //pulses.erase(pulses.begin()+ pulses.at(i));
//                    i--;
//                 }
//                    }
//                    }
//
//                    }
