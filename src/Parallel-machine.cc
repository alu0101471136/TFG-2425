/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Diseño y Análisis de Algoritmos
 * @brief Main file for the Parallel Machine Scheduling problem
 * @author alu0101471136@ull.edu.es (Raúl Álvarez)
 * @since Mar 27 2024  
 * @see {https://github.com/alu0101471136}
 */

#include "../Includes/Problem.h"
#include "../Includes/Solution.h"
#include "../Includes/Algorithms/MultiGVNS.h"
#include "../Includes/Util.h"
#include "../Includes/Instances/InstanceData.h"
#include "../Includes/Instances/Instance.h"
#include "../Includes/FLA/StudiedSolution.h"
#include <chrono>
#include <filesystem>
#include <thread>
#include <mutex>

int main(int argc, char* argv[]) {
  const std::string path = "../Instances/";
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("Directory [" + path + "] does not exist.");
  }
  if (argc <= 1) { return -1; }
  if (std::string(argv[1]) == "-h") {
    Usage();
    return 0;
  }
  thread_local Problem* thread_local_problem = nullptr;
  if (std::string(argv[1]) == "-threads") {
    std::vector<std::thread> threads; // Vector para almacenar los hilos
    std::mutex output_mutex;
    std::mutex file_mutex;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      threads.emplace_back([&, entry = entry]() {
        try {
          {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "Processing instance: " << entry.path().filename() << std::endl;
          }

          // Inicializar Problem específico para este hilo
          thread_local_problem = &Problem::getInstance(entry.path().string());
          if (!thread_local_problem) {
            throw std::runtime_error("Failed to initialize Problem instance");
          }
          int number_of_tasks = thread_local_problem->getTasksTimes().size();
          MultiGVNS multigvns(number_of_tasks);
          
          std::chrono::seconds performance_time;
          auto start = std::chrono::steady_clock::now();
          Solution solution = multigvns.Solve();
          auto end = std::chrono::steady_clock::now();
          double update_percentage;
          update_percentage = multigvns.GetUpdatePercentage();
          performance_time = std::chrono::duration_cast<std::chrono::seconds>(end - start);

          std::string instance = entry.path().filename();
          solution.PrintStudiedSolution(instance, "GVNS", performance_time.count(), Problem::getInstance().getTasksTimes().size());
          std::unique_ptr<SolutionDataTable> solution_table = std::make_unique<SolutionDataTable>(multigvns.GetSolutionDataTable());
          // {
          //   std::lock_guard<std::mutex> lock(output_mutex);
          //   solution_table->PrintTable();
          // }
          std::unique_ptr<NeighborhoodData> neighborhood_data = std::make_unique<NeighborhoodData>(multigvns.GetNeighborhoodData());
          InstanceData data;
          std::unique_ptr<InstanceData> instance_data = std::make_unique<InstanceData>(data);

          // std::cout << solution << std::endl << "Performance time: " << performance_time.count() << " seconds" << std::endl << "Update percentage: " << update_percentage << "%" << std::endl;
          // std::cout << "Neighborhood data: " << std::endl;
          // std::cout << multigvns.GetNeighborhoodData() << std::endl;
          // std::cout << "Solution data table: " << std::endl;
          // std::cout << data << std::endl;

          StudiedSolution studied_solution(instance, std::move(solution_table), std::move(neighborhood_data), std::move(instance_data));
          {
            std::lock_guard<std::mutex> lock(file_mutex);
            studied_solution.WriteCSVFile("../Results/sourceData.csv");
          }
        } catch (const std::exception& e) {
          // Manejar errores en el hilo
          std::lock_guard<std::mutex> lock(output_mutex);
          std::cerr << "Error processing instance " << entry.path().filename() << ": " << e.what() << std::endl;
        }
      });
    }

    // Esperar a que todos los hilos terminen
    for (auto& thread : threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  } else if (std::string(argv[1]) == "-all") {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      std::cout << entry.path().string() << std::endl;
      Problem& problem = Problem::getInstance(entry.path().string());
      int number_of_tasks = problem.getTasksTimes().size();
      std::string instance = entry.path().filename();
      instance = instance.substr(0, instance.find_last_of('.'));
      std::cout << instance << "  " << number_of_tasks << std::endl;
      MultiGVNS multigvns(number_of_tasks);
      auto start = std::chrono::steady_clock::now();
      Solution solution = multigvns.Solve();
      auto end = std::chrono::steady_clock::now();
      std::string algorithm_name;
      std::chrono::seconds performance_time;
      double update_percentage;
      update_percentage = multigvns.GetUpdatePercentage();
      performance_time = std::chrono::duration_cast<std::chrono::seconds>(end - start);
      solution.PrintStudiedSolution(instance, algorithm_name, performance_time.count(), Problem::getInstance().getTasksTimes().size());
      std::cout << solution << std::endl << "Performance time: " << performance_time.count() << " seconds" << std::endl << "Update percentage: " << update_percentage << "%" << std::endl;
      std::cout << "Neighborhood data: " << std::endl;
      std::cout << multigvns.GetNeighborhoodData() << std::endl;
      std::cout << "Solution data table: " << std::endl;
      std::unique_ptr<SolutionDataTable> solution_table = std::make_unique<SolutionDataTable>(multigvns.GetSolutionDataTable());
      solution_table->PrintTable();
      std::unique_ptr<NeighborhoodData> neighborhood_data = std::make_unique<NeighborhoodData>(multigvns.GetNeighborhoodData());
      InstanceData data;
      std::cout << data << std::endl;
      std::unique_ptr<InstanceData> instance_data = std::make_unique<InstanceData>(data);
      StudiedSolution studied_solution(instance, std::move(solution_table), std::move(neighborhood_data), std::move(instance_data));
      // studied_solution.WriteHeader("../Results/sourceData.csv");
      studied_solution.WriteCSVFile("../Results/sourceData.csv");
    }  
  }else if (std::string(argv[1]) == "-gen") {
    int number_of_tasks = std::stoi(argv[2]);
    int number_of_machines = std::stoi(argv[3]);
    std::string tasks_distribution = argv[4];
    std::string setup_distribution = argv[5];
    Instance instance(number_of_tasks, number_of_machines, tasks_distribution, setup_distribution);
    instance.GenerateInstance(4, 0.7);
    instance.SaveInstance(); 
  } else {
    // int algorithmOption = AlgorithmMenu();
    std::string instance = argv[1];
    Problem& problem = Problem::getInstance("../Instances/" + instance + ".txt");
    int number_of_tasks = problem.getTasksTimes().size();
    std::cout << problem << std::endl;
    Solution solution;
    std::string algorithm_name;
    std::chrono::seconds performance_time;
    double update_percentage;
    algorithm_name = "GVNS";
    MultiGVNS multigvns(number_of_tasks);
    auto start = std::chrono::steady_clock::now();
    solution = multigvns.Solve();
    auto end = std::chrono::steady_clock::now();
    update_percentage = multigvns.GetUpdatePercentage();
    performance_time = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    solution.PrintStudiedSolution(instance, algorithm_name, performance_time.count(), Problem::getInstance().getTasksTimes().size());
    std::cout << solution << std::endl << "Performance time: " << performance_time.count() << " seconds" << std::endl << "Update percentage: " << update_percentage << "%" << std::endl;
    std::cout << "Neighborhood data: " << std::endl;
    std::cout << multigvns.GetNeighborhoodData() << std::endl;
    std::cout << "Solution data table: " << std::endl;
    std::unique_ptr<SolutionDataTable> solution_table = std::make_unique<SolutionDataTable>(multigvns.GetSolutionDataTable());
    solution_table->PrintTable();
    std::unique_ptr<NeighborhoodData> neighborhood_data = std::make_unique<NeighborhoodData>(multigvns.GetNeighborhoodData());
    InstanceData data;
    std::cout << data << std::endl;
    std::unique_ptr<InstanceData> instance_data = std::make_unique<InstanceData>(data);
    StudiedSolution studied_solution(instance, std::move(solution_table), std::move(neighborhood_data), std::move(instance_data));
    // studied_solution.WriteHeader("../Results/sourceData.csv");
    studied_solution.WriteCSVFile("../Results/sourceData.csv");
  }
  return 0;
}
