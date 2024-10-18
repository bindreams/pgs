#pragma once
#include <stdexcept>

struct PgsError : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

struct PgsReadError : public PgsError {
	using PgsError::PgsError;
};

struct PgsWriteError : public PgsError {
	using PgsError::PgsError;
};
