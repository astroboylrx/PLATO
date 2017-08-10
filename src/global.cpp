//
//  global.cpp
//  PLAN: PLanetesimal ANalyzer
//
//  Created by Rixin Li on 4/26/16.
//  Copyright © 2016 Rixin Li. All rights reserved.
//

/*! \file global.cpp
 *  \brief contains function definitions for I/O-related class and utilities */

#include "global.hpp"


/*****************************************/
/********** NumericalParameters **********/
/*****************************************/

/*! \fn NumericalParameters()
 *  \brief constructor */
NumericalParameters::NumericalParameters()
{
    ;
}

/*! \fn void CalculateNewParameters()
 *  \brief calculate new parameters based on box shape */
void NumericalParameters::CalculateNewParameters()
{
    box_half_width = box_length / 2.0;
    max_half_width = 0.0;
    for (int i = 0; i != dim; i++) {
        max_half_width = std::max(max_half_width, box_half_width[i]);
    }
    shear_speed = q * Omega * box_length[0];
}



/*****************************************/
/********** Basic_IO_Operations **********/
/*****************************************/

/*! \fn Basic_IO_Operations()
 *  \brief constructor */
Basic_IO_Operations::Basic_IO_Operations()
{
    ;
}

/*! \fn int Initialize(int argc, const char * argv[])
 *  \brief initialization */
int Basic_IO_Operations::Initialize(int argc, const char * argv[])
{
    // the following manipulation of command line options is from getopb.h
    // an alternative choice is boost library: Boost.Program_options
    
    // Specifying the expected options
    static struct option long_options[] = {
        // These options set a flag
        {"Debug", no_argument, &flags.debug_flag, 1},
        {"Verbose", no_argument, &flags.verbose_flag, 1},
        {"Combined", no_argument, &flags.combined_flag, 1},
        {"Find_Clumps", no_argument, &flags.find_clumps_flag, 1},
        {"Basic_Analyses", no_argument, &flags.basic_analyses_flag, 1},
        {"Density_Vs_Scale", no_argument, &flags.density_vs_scale_flag, 1},
        {"Temp_Calculation", no_argument, &flags.tmp_calculation_flag, 1},
        {"Help", no_argument, &flags.help_flag, 1},
        // These options don't set a flag
        {"num_cpus", required_argument, nullptr, 'c'},
        {"data_dir", required_argument, nullptr, 'i'},
        {"basename", required_argument, nullptr, 'b'},
        {"postname", required_argument, nullptr, 'p'},
        {"file_num", required_argument, nullptr, 'f'},
        {"output", required_argument, nullptr, 'o'},
        {"InputConst", required_argument, nullptr, 't'},
#ifdef SMR_ON
        {"level", required_argument, 0, 'l'},
        {"domain", required_argument, 0, 'd'},
#endif // SMR_ON
        // End
        {0,0, nullptr,0}
    };
    
    log_info << "Verifying command-line-input arguments: \n";
    if (argc < 13) {
        PrintUsage(argv[0]);
    } else {
        int tmp_option;
        while (1) {
            // getopt_long stores the option
            int option_index = 0;
#ifndef SMR_ON
            // remember add ":" after the letter means this option has argument after it
            tmp_option = getopt_long(argc, (char *const *)argv, ":c:i:b:p:f:o:t:h", long_options, &option_index);
#else // SMR_ON
            tmp_option = getopt_long(argc, (char *const *)argv, ":c:i:b:p:f:o:t:hl:d:", long_options, &option_index);
#endif // SMR_ON
            if (tmp_option == -1) {
                break;
            }
            
            switch (tmp_option) {
                case 0: {
                    log_info << "Set flag \"" << long_options[option_index].name << "\"";
                    //if (optarg) {
                    //    log_info << " with arg " << optarg;
                    //}
                    log_info << std::endl;
                    // if we don't need output: "this option set a flag, do nothing else now"
                    if (long_options[option_index].flag != 0) {
                        break;
                    } else {
                        error_message << "Error: something wrong with non-flag option " << long_options[option_index].name << " which might be supposed to be a flag." << std::endl;
                        Output(std::cerr, error_message, __normal_output, __master_only);
                    }
                }
                case 'c': {
                    std::istringstream tmp_iss;
                    tmp_iss.str(optarg);
                    tmp_iss >> num_cpus;
                    log_info << "num_cpus is " << num_cpus << "\n";
                    break;
                }
                case 'i': {
                    file_name.data_file_dir.assign(optarg);
                    log_info << "data_file_dir is " << file_name.data_file_dir << "\n";
                    break;
                }
                case 'b': {
                    file_name.data_file_basename.assign(optarg);
                    log_info << "data_file_basename is " << file_name.data_file_basename << "\n";
                    break;
                }
                case 'p': {
                    file_name.data_file_postname.assign(optarg);
                    log_info << "data_file_postname is " << file_name.data_file_postname << "\n";
                    break;
                }
                case 'f': {
                    std::string tmp_str;
                    tmp_str.assign(optarg);
                    size_t pos1 = tmp_str.find_first_of(':');
                    size_t pos2 = tmp_str.find_last_of(':');
                    std::istringstream tmp_iss;
                    char tmp_char;
                    if (pos1 == pos2) {
                        tmp_iss.str(optarg);
                        tmp_iss >> start_num >> tmp_char >> end_num;
                        interval = 1;
                    } else {
                        tmp_iss.str(tmp_str.substr(0, pos2));
                        tmp_iss >> start_num >> tmp_char >> end_num;
                        std::istringstream tmp_iss_interval;
                        tmp_iss_interval.str(tmp_str.substr(pos2+1));
                        tmp_iss_interval >> interval;
                    }
                    if (start_num < 0) {
                        error_message << "The start number should be positive (Auto fix to 0)" << std::endl;
                        Output(std::cerr, error_message, __normal_output, __master_only);
                        start_num = 0;
                    }
                    if (end_num < start_num) {
                        error_message << "The end number should be larger or equal than the start number. (Auto fix to start number)." << std::endl;
                        Output(std::cerr, error_message, __normal_output, __master_only);
                        end_num = start_num;
                    }
                    if (interval == 0) {
                        error_message << "The interval should be non-zero. (Auto fix to 1)" << std::endl;
                        Output(std::cerr, error_message, __normal_output, __master_only);
                        interval = 1;
                    }
                    num_files = (end_num - start_num) / interval + 1;
                    log_info << "start_num = " << start_num << ", end_num = " << end_num << ", interval = " << interval << ", num_files = " << num_files << "\n";
                    break;
                } // case 'f'
                case 'o': {
                    file_name.output_file_path.assign(optarg);
                    log_info << "output_file_path is " << file_name.output_file_path << "\n";
                    break;
                }
                case 't': {
                    file_name.input_const_path.assign(optarg);
                    log_info << "input_const_path is " << file_name.input_const_path << "\n";
                    break;
                }
                case 'h':{
                    flags.help_flag = 1;
                    break;
                }
#ifdef SMR_ON
                case 'l': {
                    file_name.data_level.assign(optarg);
                    log_info << "data_level is " << file_name.data_level << "\n";
                    break;
                }
                case 'd': {
                    file_name.data_domain.assign(optarg);
                    log_info << "data_domain is " << file_name.data_domain << "\n";
                    break;
                }
#endif // SMR_ON
#if 0 // comment this out to enable case 1
                case 1:
                    /*
                     * Use this case if getopt_long() should go through all arguments. If so, add a leading '-' character to optstring.
                     * Actual code, if any, goes here.
                     */
                    break;
#endif // 0
                case ':': { // missing option argument (to enable case ':', put a leading ':' in optstring
                    error_message << "Error: Option -" << optopt << " requires an arugment.\n";
                    Output(std::cerr, error_message, __normal_output, __master_only);
                    exit(2); // cannot execute
                }
                case '?': // invalid option
                default: {
                    if (!isprint(optopt)) {
                        error_message << "Error: Unknown/invalid flag, check usage with --Help or -h.\n";
                        Output(std::cerr, error_message, __normal_output, __master_only);
                    } else {
                        error_message << "Error: Unknown/invalid option -" << static_cast<char>(optopt) << "\n";
                        Output(std::cerr, error_message, __normal_output, __master_only);
                    }
                    exit(2); // cannot execute
                }
            }
        }
        
        // check if users need help
        if (flags.help_flag) {
            PrintUsage(argv[0]);
            exit(0);
        }
        // determine ostream level before first Output()
        if (flags.verbose_flag) {
            ostream_level = __more_output;
        }
        if (flags.debug_flag) {
            ostream_level = __even_more_output;
        }
        
        // if no serious flags, set find_clumps_flag to 1
        if (!flags.find_clumps_flag && !flags.basic_analyses_flag && !flags.density_vs_scale_flag) {
            flags.find_clumps_flag = 1;
        }
        
        Output(std::clog, log_info, __more_output, __master_only);
        PrintStars(std::clog, __more_output);
        if (optind < argc) {
            error_message << "Non-option ARGV-elements: ";
            while (optind < argc) {
                error_message << argv[optind++];
            }
            error_message << "\nSomething is wrong with the argument list. Check usage by --Help or -h.\n";
            Output(std::cerr, error_message, __normal_output, __master_only);
            exit(2); // cannot execute
        }
    } // if (argc < 13)
    
    physical_quantities.resize(num_files);
    GenerateFilenames();
    
    return 0;
}

/*! \fn void PrintUsage(const char *program_name)
 *  \brief print usage if required argument is missing */
void Basic_IO_Operations::PrintUsage(const char *program_name)
{
    out_content << "USAGE: \n" << program_name
    << " -c <num_cpus> -i <data_dir> -b <basename> -p <postname>  -f <range(f1:f2)|range_step(f1:f2:step)> -o <output> [-t <input_file_for_constants> --flags]\n"
    << "Example: ./plan -c 64 -i ./bin/ -b Par_Strat3d -p ds -f 170:227 -o result.txt --Verbose\n"
    << "[...] means optional arguments. Available flags: \n"
    << "Use --Help to obtain this usage information\n"
    << "Use --Verbose to obtain more output during execution\n"
    << "Use --Debug to obtain all possible output during execution\n"
    << "Use --Combined to deal with combined lis files (from all processors)\n"
    << "Use --Find_Clumps to run clump finding functions\n"
    << "Use --Basic_Analyses to perform basic analyses, which will output max($\\rho_p$) and $H_p$\n"
    << "Use --Density_Vs_Scale to calculate max($\\rho_p$) as a function of length scales\n"
    //<< "Use --Temp_Calculation to do temporary calculations in TempCalculation()\n"
    << "If you don't specify any flags, then --Find_Clumps will be turned on automatically.";
    out_content << std::endl;
    Output(std::cout, out_content, __normal_output, __master_only);
    exit(0); // cannot execute
}

/*! \fn void Output(std::ostream &stream, std::ostringstream &content, const OutputLevel &output_level, const MPI_Level &mpi_level)
 *  \brief handle output by log level & MPI status */
void Basic_IO_Operations::Output(std::ostream &stream, std::ostringstream &content, const OutputLevel &output_level, const MPI_Level &mpi_level)
{
    if (ostream_level >= output_level) {
#ifdef MPI_ON
        if (mpi_level == __master_only) {
            if (mpi->myrank == mpi->master) {
                stream << content.str() << std::flush;
            }
        } else if (mpi_level == __all_processors) {
            stream << mpi->RankInfo() << content.str() << std::flush;
        }
#else // MPI_ON
        stream << content.str() << std::flush;
#endif // MPI_ON
    }
    Reset(content);
}

/*! \fn std::string LocalTime()
 *  \brief return a string containing the date and time information */
std::string Basic_IO_Operations::LocalTime()
{
    std::time_t current_local_time = std::time(nullptr);
    std::string tmp_str = std::asctime(std::localtime(&current_local_time));
    tmp_str.pop_back(); // remove the last '\n' character
    return tmp_str;
}

/*! \fn void PrintStars(std::ostream &stream, const OutputLevel &output_level)
 *  \brief print 80 * symbols as a divider line */
void Basic_IO_Operations::PrintStars(std::ostream &stream, const OutputLevel &output_level)
{
    out_content << std::setw(80) << std::setfill('*') << "*\n";
    Output(stream, out_content, output_level, __master_only);
}

/*! \fn void GenerateFilenames()
 *  \brief generate the name of data files for processing */
void Basic_IO_Operations::GenerateFilenames()
{
    if (*file_name.data_file_dir.rbegin() != '/') {
        file_name.data_file_dir.push_back('/');
    }
    
    if (flags.combined_flag) {
        file_name.lis_data_file_name.reserve(num_files);
        file_name.vtk_data_file_name.reserve(num_files);
        log_info << "Verifying generated data file names (only the first one and last one):\n";
        
        for (int num = start_num; num != end_num+interval; num += interval) {
            std::stringstream formatted_num;
            formatted_num << std::setw(4) << std::setfill('0') << num;
            
            file_name.lis_data_file_name.push_back(file_name.data_file_dir+file_name.data_file_basename+"."+formatted_num.str()+"."+file_name.data_file_postname+".lis");
            file_name.vtk_data_file_name.push_back(file_name.data_file_dir+file_name.data_file_basename+"."+formatted_num.str()+".vtk");
            if (num == start_num || num == end_num) {
                log_info << file_name.lis_data_file_name.back() << "\n" << file_name.vtk_data_file_name.back() << "\n";
            }
            if (num > end_num + interval) {
                exit(4); // wrong function argument
            }
        }
        
    } else {
        file_name.lis_data_file_name.reserve(num_files * num_cpus);
        file_name.vtk_data_file_name.reserve(num_files * num_cpus);
        log_info << "Verifying generated data file names (only id0 and id[max]):\n";
        
        for (int num = start_num; num != end_num+interval; num += interval) {
            std::stringstream formatted_num;
            formatted_num << std::setw(4) << std::setfill('0') << num;
            
            file_name.lis_data_file_name.push_back(file_name.data_file_dir+"id0/"+file_name.data_file_basename+"."+formatted_num.str()+"."+file_name.data_file_postname+".lis");
            file_name.vtk_data_file_name.push_back(file_name.data_file_dir+"id0/"+file_name.data_file_basename+"."+formatted_num.str()+".vtk");
            log_info << file_name.lis_data_file_name.back() << "\n" << file_name.vtk_data_file_name.back() << "\n";
            for (int id = 1; id != num_cpus; id++) {
                file_name.lis_data_file_name.push_back(file_name.data_file_dir+"id"+std::to_string(id)+"/"+file_name.data_file_basename+"-id"+std::to_string(id)+"."+formatted_num.str()+"."+file_name.data_file_postname+".lis");
                file_name.vtk_data_file_name.push_back(file_name.data_file_dir+"id"+std::to_string(id)+"/"+file_name.data_file_basename+"-id"+std::to_string(id)+"."+formatted_num.str()+".vtk");
            }
            log_info << file_name.lis_data_file_name.back() << "\n" << file_name.vtk_data_file_name.back() << "\n";
            if (num > end_num + interval) {
                exit(4); // wrong function argument
            }
        }
    }
    
    file_name.max_rhop_vs_scale_file = file_name.output_file_path.substr(0, file_name.output_file_path.find_last_of('.'))+"_RMPL.txt";
    file_name.mean_sigma_file = file_name.output_file_path.substr(0, file_name.output_file_path.find_last_of('.'))+"_MeanSigma.txt";
    file_name.planetesimals_file = file_name.output_file_path.substr(0, file_name.output_file_path.find_last_of('.'))+"_planetesimals.txt";
    
    Output(std::clog, log_info, __even_more_output, __master_only);
    PrintStars(std::clog, __even_more_output);
    
}

/*! \fn ~Basic_IO_Operations()
 *  \brief destructor */
Basic_IO_Operations::~Basic_IO_Operations()
{
    ;
}

/*********************************/
/********** MPI_Wrapper **********/
/*********************************/

/*! \fn MPI_Wrapper()
 *  \brief constructor */
MPI_Wrapper::MPI_Wrapper()
{
    ;
}

/*! \fn void Initialize(int argc, const char * argv[])
 *  \brief MPI initializaion */
void MPI_Wrapper::Initialization(int argc, const char * argv[])
{
#ifdef MPI_ON
    // common initialization of MPI
    MPI_Init(&argc, (char***)&argv);
    world = MPI_COMM_WORLD;
    MPI_Comm_size(world, &num_processors);
    MPI_Comm_rank(world, &myrank);
#else // MPI_ON
    num_processors = 1;
    myrank = 0;
#endif // MPI_ON
    master = 0;
    // initialize file loop's parameters
    loop_begin = myrank;
    loop_end = myrank;
    loop_step = num_processors;
    
    timer[__waiting_time].StartTimer();
    timer[__waiting_time].StopTimer();
    // further waitting time for each processor should use ResumeTimer()
}

/*! \fn void Determine_Loop(int num_files)
 *  \brief determine the begin/end/offset for file loop */
void MPI_Wrapper::DetermineLoop(int num_files)
{
    // use reverse order
    loop_begin = num_processors - 1 - myrank;
    loop_end = num_files - 1;
    if (loop_begin >= num_processors) {
        loop_end = -1;
    }
#ifdef MPI_ON
    if (num_files < num_processors) {
        MPI_Group world_group, file_writing_group;
        MPI_Comm_group(world, &world_group);
        int *chosen_ranks = new int[num_files];
        int tmp_myrank = 0;

        for (int i = num_processors - num_files, j = 0; i != num_processors; i++, j++) {
            chosen_ranks[j] = i;
        }
        MPI_Group_incl(world_group, num_files, chosen_ranks, &file_writing_group);
        MPI_Comm_create(world, file_writing_group, &file_writing_communicator);
        // for processors with myrank >= num_processors-num_files, their file_writing_communicator is a new communicator; for others, it is MPI_COMM_NULL, which will cause errors if you use it as a real communicator
        if (file_writing_communicator != MPI_COMM_NULL) {
            MPI_Comm_rank(file_writing_communicator, &tmp_myrank);
            if (tmp_myrank != 0) {
                tmp_myrank = 0;
            }else {
                tmp_myrank = myrank;
            }
        }
        // to get who is file_writing_master, important for writing file header if num_files < num_processors
        MPI_Allreduce(&tmp_myrank, &file_writing_master, 1, MPI_INT, MPI_SUM, world);
    } else {
        file_writing_communicator = world;
        file_writing_master = master;
    }
#endif
}

/*! \fn void Barrier()
 *  \brief a wrapper of MPI Barrier */
void MPI_Wrapper::Barrier()
{
#ifdef MPI_ON
    timer[__waiting_time].ResumeTimer();
    MPI_Barrier(world);
    timer[__total_elapse_time].StopTimer();
#endif // MPI_ON
}

/*! \fn std::string RankInfo()
 *  \brief return a string contains "Processor myrank: " */
std::string MPI_Wrapper::RankInfo()
{
#ifdef MPI_ON
    std::ostringstream oss;
    oss << "Processor " << myrank << ": ";
    return oss.str();
#else // MPI_ON
    return std::string();
#endif // MPI_ON
}

/*! \fn void Finalize()
 *  \brief a wrapper of MPI Finalize() */
void MPI_Wrapper::Finalize()
{
#ifdef MPI_ON
    double max_waiting_time = 0, waiting_time = timer[__waiting_time].GiveTime();
    MPI_Reduce(&waiting_time, &max_waiting_time, 1, MPI_DOUBLE, MPI_MAX, master, world);
    progIO->log_info << "Max waiting time among all processors due to Barrier(): " << max_waiting_time << "s." << std::endl;
    progIO->Output(std::clog, progIO->log_info, __normal_output, __master_only);
    MPI_Finalize();
#endif // MPI_ON
}

/*! \fn void OpenFile(std::string filename)
 *  \brief open file for data writing */
void MPI_Wrapper::OpenFile(std::string file_name)
{
#ifdef MPI_ON
    /*
     * Notice that using MPI_File_open to open a file with MPI_MODE_CREATE won't clobber the file if it already exists. Thus you need to either use MPI_MODE_EXCL or MPI_MODE_DELETE_ON_CLOSE to overcome it.
     */
    MPI_File tmp_file;
    result_files.push_back(tmp_file);
    file_pos[file_name] = result_files.size() - 1;
    
    if (MPI_File_open(world, file_name.c_str(), MPI_MODE_CREATE|MPI_MODE_WRONLY|MPI_MODE_EXCL, MPI_INFO_NULL, &result_files.at(file_pos[file_name]))) {
        if (myrank == master) {
            MPI_File_delete(file_name.c_str(), MPI_INFO_NULL);
        }
        if (MPI_File_open(world, file_name.c_str(), MPI_MODE_CREATE|MPI_MODE_WRONLY|MPI_MODE_EXCL, MPI_INFO_NULL, &result_files.at(file_pos[file_name]))) {
            progIO->error_message << "Error: Failed to open file " << file_name << std::endl;
            progIO->Output(std::cerr, progIO->error_message, __normal_output, __all_processors);
            exit(3); // cannot open file
        }
    }
    offset[result_files.at(file_pos[file_name])] = 0;
    header_offset[result_files.at(file_pos[file_name])] = 0;
#else // MPI_ON
    
    result_files.push_back(std::ofstream(file_name.c_str(), std::ofstream::out));
    file_pos[file_name] = result_files.size() - 1;
    
    if (!(result_files.at(file_pos[file_name])).is_open()) {
        progIO->error_message << "Error: Failed to open file " << file_name << std::endl;
        progIO->Output(std::cerr, progIO->error_message, __normal_output, __all_processors);
        exit(3); // cannot open file
    }
#endif // MPI_ON
}

/*! \fn void WriteFile(file_obj &__file, std::ostringstream &content, const int loop_count)
 *  \brief all processor write into a file, use with cautions -> read the assumptions in descriptions
 *  When MPI_ON is on, this function assumes that you only write file header if specifying __master_only, and it assumes that every processor are writing the same amount of chunk into the file every time */
void MPI_Wrapper::WriteSingleFile(file_obj &__file, std::ostringstream &content, const MPI_Level &mpi_level)
{
#ifdef MPI_ON
    std::string tmp_str = content.str();
    if (mpi_level == __master_only) {
        header_offset[__file] += tmp_str.size();
        MPI_Bcast(&header_offset[__file], 1, MPI_OFFSET, master, file_writing_communicator); // Bcast from 0, still
        if (myrank == file_writing_master) {
            const char *tmp_char = tmp_str.data();
            MPI_File_write(__file, tmp_char, static_cast<int>(tmp_str.size()), MPI_CHAR, &status);
        } else {
            MPI_File_seek(__file, header_offset[__file], MPI_SEEK_SET);
        }
        MPI_File_sync(__file);
    } else {
        if (loop_end == -1) {
            return; // this should not happen
        }
        MPI_File_get_position(__file, &offset[__file]);
        if (offset[__file] == header_offset[__file]) {
            MPI_File_seek(__file, tmp_str.size()*(loop_begin), MPI_SEEK_CUR);
        } else {
            MPI_File_seek(__file, tmp_str.size()*(loop_step-1), MPI_SEEK_CUR);
        }
        const char *tmp_char = tmp_str.data();
        MPI_File_write(__file, tmp_char, static_cast<int>(tmp_str.size()), MPI_CHAR, &status);
        MPI_File_sync(__file);
    }
#else // MPI_ON
    __file << content.str();
#endif // MPI_ON
    progIO->Reset(content);
}

/*! \fn void CloseFile(file_obj &__file, )
 *  \brief close the file for data writing */
void MPI_Wrapper::CloseFile(file_obj &__file)
{
#ifdef MPI_ON
    //MPI_File_sync(__file);
    MPI_File_close(&__file);
#else // MPI_ON
    __file.close();
#endif // MPI_ON
}

/*! \fn ~MPI_info()
 *  \brief a destructor */
MPI_Wrapper::~MPI_Wrapper()
{
    ;
}

/***************************/
/********** Timer **********/
/***************************/

/*! \fn Timer()
 *  \brief constructor */
Timer::Timer()
{
    timer_on_flag = 0;
    skip_time = 0;
}

/*! \fn double GetCurrentTime()
 *  \brief get current time */
double Timer::GetCurrentTime()
{
#ifdef MPI_ON
    return MPI_Wtime();
#else // MPI_ON
    return double(clock())/CLOCKS_PER_SEC;
#endif // MPI_ON
}

/*! \fn void StartTimer()
 *  \brief start the timer */
void Timer::StartTimer()
{
    ClearTimer();
    begin_time = GetCurrentTime();
    timer_on_flag = 1;
}

/*! \fn int void()
 *  \brief record a lap time and return index */
int Timer::Lap()
{
    if (timer_on_flag) {
        lap_time.push_back(GetCurrentTime() - begin_time - skip_time);
        return (unsigned int)(lap_time.size()-1);
    } else {
        std::cerr << "Error: Timer is off. Cannot lap a time." << std::endl;
        return -1;
    }
}

/*! \fn void StopTimer()
 *  \brief stop the timer and return time */
void Timer::StopTimer()
{
    stop_time = GetCurrentTime() - begin_time - skip_time;
    lap_time.push_back(stop_time);
    timer_on_flag = 0;
}

/*! \fn void ClearTimer()
 *  \brief reset the timer */
void Timer::ClearTimer()
{
    begin_time = 0;
    timer_on_flag = 0;
    skip_time = 0;
    stop_time = 0;
    std::vector<double> TempVector;
    lap_time.swap(TempVector);
}

/*! \fn void ResumeTimer()
 *  \brief resume the timer */
void Timer::ResumeTimer()
{
    skip_time += GetCurrentTime() - (stop_time+begin_time+skip_time);
    timer_on_flag = 1;
    stop_time = 0;
}

/*! \fn double GiveTime()
 *  \brief show the timer */
double Timer::GiveTime()
{
    if (timer_on_flag) {
        return GetCurrentTime() - begin_time - skip_time;
    } else {
        return stop_time;
    }
}

/*! \fn double GiveTime(const unsigned int i)
 *  \brief give a lap time */
double Timer::GiveTime(const unsigned int i)
{
    if (i < lap_time.size()) {
        return lap_time[i];
    } else {
        std::cerr << "Error: There is not such a lap time record." << std::endl;
        return -1;
    }
}

/*! \fn ~Timer()
 *  \brief destructor */
Timer::~Timer()
{
    std::vector<double> TempVector;
    lap_time.swap(TempVector);
}