/**
 * \file macros.h
 *
 * \brief Macros for the ToF and AoA applications
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */
#pragma once

#include <iostream>

#define J (std::complex<double>(0,1))

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
/**
 * \def TRACE
 *
 * Used for debugging. Simply creates an "I am here!" string.
 *
 * \ingroup group_Macros
 */
#define TRACE "Reached: " __FILE__ ":" TOSTRING(__LINE__)
/**
 * \def MARK
 *
 * Used for debugging. Simply prints an "I am here!" statement to cout.
 * See: http://www.decompile.com/cpp/faq/file_and_line_error_string.htm
 *
 * \ingroup group_Macros
 */
#define MARK std::cout << TRACE << std::endl;
