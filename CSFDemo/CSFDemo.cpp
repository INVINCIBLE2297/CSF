// ======================================================================================
// Copyright 2017 State Key Laboratory of Remote Sensing Science, 
// Institute of Remote Sensing Science and Engineering, Beijing Normal University

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ======================================================================================
#define ONE_FILE 0
#define FILES 1
#define MODE ONE_FILE

#define ONETHREAD 0
#define MULTITHREAD 1
#define THREAD ONETHREAD


#include <vector>
#include "Cfg.h"
#include "../src/CSF.h" 
#include <locale.h>
#include <time.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#if (MODE == ONEFILE)
int txt2pcd(const std::string &filename);
#elif (MODE == FILES)
#include <filesystem>
int txt2pcd(const std::string &input_filename,const std::string &output_filename);
std::vector<std::string> get_file_paths(const std::string& directory_path);
#endif
#if (THREAD == MULTITHREAD)
#include <thread>
#include <atomic>
#include <chrono> // 新增头文件
#include <mutex>  // 新增头文件
#endif
int main(int argc,char* argv[])
{
	Cfg cfg;
	std::string slop_smooth;
	cfg.readConfigFile("params.cfg", "slop_smooth", slop_smooth);
	bool ss = false;
	if (slop_smooth == "true" || slop_smooth == "True")
	{
		ss = true;
	}
	else if (slop_smooth == "false" || slop_smooth == "False")
	{
		ss = false;
	}
	else{
		if (atoi(slop_smooth.c_str()) == 0){
			ss = false;
		}
		else
		{
			ss = true;
		}
	}

	std::string class_threshold;
	cfg.readConfigFile("params.cfg", "class_threshold", class_threshold);
	std::string cloth_resolution;
	cfg.readConfigFile("params.cfg", "cloth_resolution", cloth_resolution);
	std::string iterations;
	cfg.readConfigFile("params.cfg", "iterations", iterations);
	std::string rigidness;
	cfg.readConfigFile("params.cfg", "rigidness", rigidness);
	std::string time_step;
	cfg.readConfigFile("params.cfg", "time_step", time_step);
#if (MODE == ONE_FILE)
	std::string terr_pointClouds_filepath;
	cfg.readConfigFile("params.cfg", "terr_pointClouds_filepath", terr_pointClouds_filepath);
	CSF csf;
	//step 1 Input the point cloud
	csf.readPointsFromFile(terr_pointClouds_filepath);

	clock_t start, end;
	start = clock();

	//In real application, the point cloud may be provided by main program
	//csf.setPointCloud(pc);//pc is PointCloud class

	//step 2 parameter setting
	csf.params.bSloopSmooth = ss;
	csf.params.class_threshold = atof(class_threshold.c_str());
	csf.params.cloth_resolution = atof(cloth_resolution.c_str());
	csf.params.interations = atoi(iterations.c_str());
	csf.params.rigidness = atoi(rigidness.c_str());
	csf.params.time_step = atof(time_step.c_str());

	//step3 do filtering
	std::vector<int> groundIndexes, offGroundIndexes;
	if (argc == 2 && strcmp(argv[1], "-c")==0)
	{
		std::cout << "Export cloth enabled." << std::endl;
		csf.do_filtering(groundIndexes, offGroundIndexes, true);
	}
	else
	{
		csf.do_filtering(groundIndexes, offGroundIndexes, false);
	}
		

	end = clock();
	double dur = (double)(end - start);
	printf("Use Time:%f\n", (dur / CLOCKS_PER_SEC));

	csf.savePoints(groundIndexes,"ground.txt");
	csf.savePoints(offGroundIndexes, "map.txt");
	txt2pcd("ground");
	txt2pcd("map");
#elif (MODE == FILES)
#if (THREAD == ONETHREAD)
	int count = 0;
	std::vector<std::string> file_paths;
	std::string directory_path = "/home/tcet/dddmr_bags/ground_floor_CASIA/pcd";
	std::string directory_path_output = "/home/tcet/dddmr_bags/ground_floor_CASIA/output/";
	std::string map_path,ground_path;
	auto files = get_file_paths(directory_path);
	std::cout << "找到 " << files.size() << " 个文件：" << std::endl;
	clock_t start, end;
	start = clock();
	for (const auto& file_path : files) {
		CSF csf;
		//step 1 Input the point cloud
		csf.readPointsFromFile(file_path);

		//step 2 parameter setting
		csf.params.bSloopSmooth = ss;
		csf.params.class_threshold = atof(class_threshold.c_str());
		csf.params.cloth_resolution = atof(cloth_resolution.c_str());
		csf.params.interations = atoi(iterations.c_str());
		csf.params.rigidness = atoi(rigidness.c_str());
		csf.params.time_step = atof(time_step.c_str());

		//step3 do filtering
		std::vector<int> groundIndexes, offGroundIndexes;
		if (argc == 2 && strcmp(argv[1], "-c")==0)
		{
			std::cout << "Export cloth enabled." << std::endl;
			csf.do_filtering(groundIndexes, offGroundIndexes, true);
		}
		else
		{
			csf.do_filtering(groundIndexes, offGroundIndexes, false);
		}
		
		map_path = directory_path_output + std::to_string(count) + "_feature.pcd";
		ground_path = directory_path_output + std::to_string(count) + "_ground.pcd";
		count++;
		
		csf.savePoints(groundIndexes,"ground.txt");
		csf.savePoints(offGroundIndexes, "map.txt");
		txt2pcd("map.txt",map_path);
		txt2pcd("ground.txt",ground_path);
		
		end = clock();
		double dur = (double)(end - start);
		double et = files.size()*dur/count;
		printf("Used Time:%f, Estimated Time:%f\n", (dur / CLOCKS_PER_SEC), (et / CLOCKS_PER_SEC));
    }
#elif (THREAD == MULTITHREAD)
	std::mutex print_mutex;
	std::vector<std::string> file_paths;
	std::string directory_path = "/home/tcet/dddmr_bags/ground_floor_CASIA/pcd";
	std::string directory_path_output = "/home/tcet/dddmr_bags/ground_floor_CASIA/output/";
	std::string directory_path_buf = "/home/tcet/dddmr_bags/ground_floor_CASIA/buf/";
	std::string map_path,ground_path;
	auto files = get_file_paths(directory_path);
    std::cout << "找到 " << files.size() << " 个文件：" << std::endl;
    int filenum = files.size();
	std::atomic<int> count(0); // 线程安全的计数器（初始化为0）
	auto start = std::chrono::steady_clock::now(); // 在 main() 开头
    // 1. 定义一个处理单个文件的 Lambda 函数
    // 注意：我们将配置参数通过值传递捕获，确保线程安全
    auto processFile = [ss, class_threshold, cloth_resolution, iterations, rigidness, time_step, directory_path_output, directory_path_buf, start, filenum, &count, &print_mutex](const std::string& file_path, int file_id) {
        // --- 下面是原 for 循环内的逻辑，现在在独立线程中运行 ---
        
        CSF csf; // 每个线程必须有自己的 CSF 实例！不能共用全局变量
        
        // Step 1: 输入点云
        csf.readPointsFromFile(file_path);

        // Step 2: 参数设置 (每个线程独立设置)
        csf.params.bSloopSmooth = ss;
        csf.params.class_threshold = atof(class_threshold.c_str());
        csf.params.cloth_resolution = atof(cloth_resolution.c_str());
        csf.params.interations = atoi(iterations.c_str()); // 注意：原代码此处拼写为 interations，建议检查
        csf.params.rigidness = atoi(rigidness.c_str());
        csf.params.time_step = atof(time_step.c_str());

        // Step 3: 滤波
        std::vector<int> groundIndexes, offGroundIndexes;
        // 注意：这里 argv 相关的判断在多线程中可能需要调整，建议将 -c 选项作为全局 bool 传入
        csf.do_filtering(groundIndexes, offGroundIndexes, false); 

        // 生成输出路径
        std::string map_path = directory_path_output + std::to_string(file_id) + "_feature.pcd";
        std::string ground_path = directory_path_output + std::to_string(file_id) + "_ground.pcd";

        // 保存中间 txt (注意：如果多个线程写同一个名字的 txt 会冲突，所以必须用 file_id 区分)
        std::string temp_ground_txt = directory_path_buf + "ground_" + std::to_string(file_id) + ".txt";
        std::string temp_map_txt = directory_path_buf + "map_" + std::to_string(file_id) + ".txt";
        
        csf.savePoints(groundIndexes, temp_ground_txt.c_str());
        csf.savePoints(offGroundIndexes, temp_map_txt.c_str());

        // 转换为 PCD
        txt2pcd(temp_map_txt, map_path);
        txt2pcd(temp_ground_txt, ground_path);

        // 可选：删除临时 txt 文件
        // std::remove(temp_ground_txt.c_str()); ...

        // 线程安全的递增（原子操作）
		int current_count = ++count; // 返回递增后的值

		auto now = std::chrono::steady_clock::now();
		auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
		double et = 0.0;
		if (current_count > 0) {
			et = dur * (filenum - current_count) / current_count; // 单位：毫秒
		}
		std::lock_guard<std::mutex> lock(print_mutex);
		printf("线程 %d: 已处理 %d/%d | 估算剩余时间: %.2f 秒\n", 
			file_id, 
			current_count, 
			filenum, 
			et / 1000.0); // 转换为秒
    };

    // 2. 创建线程容器
    std::vector<std::thread> threads;
    

    // 3. 启动线程 (注意：这里假设 files.size() 不会太大)
    for (size_t i = 0; i < files.size(); ++i) {
        // 为每个文件启动一个线程
        // 注意：std::ref 如果需要引用传递，这里我们用值传递字符串
        threads.emplace_back(processFile, files[i], static_cast<int>(i));
    }

    // 4. 等待所有线程结束
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join(); // 阻塞，直到线程结束
        }
    }

    auto now = std::chrono::steady_clock::now();
	auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    printf("多线程总耗时: %.2f 秒\n", (dur / 1000.0));
#endif
#endif
	return 0;
}



#if (MODE == ONE_FILE)
int txt2pcd(const std::string &filename) {
    // 1. 读取ground.txt文件并计算行数
	std::string input_filename = filename+".txt";
    std::ifstream input(input_filename);
    if (!input.is_open()) {
        std::cerr << "无法打开" << input_filename << "文件" << std::endl;
        return 1;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    input.close();
	std::string output_filename = filename+".pcd";
    // 2. 创建ground.pcd文件
    std::ofstream output(output_filename);
    if (!output.is_open()) {
        std::cerr << "无法创建"<<output_filename<<"文件" << std::endl;
        return 1;
    }

    // 3. 写入PCD头部信息
    int pointCount = lines.size();
    output << "# .PCD v0.7 - Point Cloud Data file format\n";
    output << "VERSION 0.7\n";
    output << "FIELDS x y z intensity\n";
    output << "SIZE 4 4 4 4\n";
    output << "TYPE F F F F\n";
    output << "COUNT 1 1 1 1\n";
    output << "WIDTH " << pointCount << "\n";
    output << "HEIGHT 1\n";
    output << "VIEWPOINT 0 0 0 1 0 0 0\n";
    output << "POINTS " << pointCount << "\n";
    output << "DATA ascii\n";

    // 4. 处理并写入数据行
    for (const auto& l : lines) {
        output << l << " 0\n";
    }

    output.close();
    std::cout << "成功创建"<<output_filename<<"文件,包含" << pointCount << "个点" << std::endl;
    return 0;
}

#elif (MODE == FILES)
int txt2pcd(const std::string &input_filename,const std::string &output_filename) {
    // 1. 读取ground.txt文件并计算行数
    std::ifstream input(input_filename);
    if (!input.is_open()) {
        std::cerr << "无法打开" << input_filename << "文件" << std::endl;
        return 1;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    input.close();
    // 2. 创建ground.pcd文件
    std::ofstream output(output_filename);
    if (!output.is_open()) {
        std::cerr << "无法创建"<<output_filename<<"文件" << std::endl;
        return 1;
    }

    // 3. 写入PCD头部信息
    int pointCount = lines.size();
    output << "# .PCD v0.7 - Point Cloud Data file format\n";
    output << "VERSION 0.7\n";
    output << "FIELDS x y z intensity\n";
    output << "SIZE 4 4 4 4\n";
    output << "TYPE F F F F\n";
    output << "COUNT 1 1 1 1\n";
    output << "WIDTH " << pointCount << "\n";
    output << "HEIGHT 1\n";
    output << "VIEWPOINT 0 0 0 1 0 0 0\n";
    output << "POINTS " << pointCount << "\n";
    output << "DATA ascii\n";

    // 4. 处理并写入数据行
    for (const auto& l : lines) {
        output << l << " 0\n";
    }

    output.close();
    return 0;
}
namespace fs = std::filesystem;
std::vector<std::string> get_file_paths(const std::string& directory_path) {
    std::vector<std::string> file_paths;
    
    try {
        // 检查路径是否存在且是目录
        if (!fs::exists(directory_path)) {
            std::cerr << "错误：路径不存在 - " << directory_path << std::endl;
            return file_paths;
        }
        if (!fs::is_directory(directory_path)) {
            std::cerr << "错误：指定路径不是目录 - " << directory_path << std::endl;
            return file_paths;
        }

        // 遍历目录中的所有条目
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            // 只添加文件（跳过子目录）
            if (fs::is_regular_file(entry.status())) {
                file_paths.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "文件系统错误: " << e.what() << std::endl;
    }
    
    return file_paths;
}
#endif