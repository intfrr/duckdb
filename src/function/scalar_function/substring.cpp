#include "function/scalar_function/substring.hpp"

#include "common/exception.hpp"
#include "common/vector_operations/vector_operations.hpp"

using namespace std;

namespace duckdb {
namespace function {

void substring_function(Vector inputs[], size_t input_count, Expression &expr, Vector &result) {
	assert(input_count == 3);
	auto &input = inputs[0];
	auto &offset = inputs[1];
	auto &length = inputs[2];
	assert(input.type == TypeId::VARCHAR);

	result.Initialize(TypeId::VARCHAR);
	result.nullmask = input.nullmask;

	auto result_data = (const char **)result.data;
	auto input_data = (const char **)input.data;
	auto offset_data = (int *)offset.data;
	auto length_data = (int *)length.data;

	size_t max_str_len = expr.children[2]->stats.max.GetNumericValue();

	auto output_uptr = unique_ptr<char[]>{new char[max_str_len + 1]};
	char *output = output_uptr.get();

	VectorOperations::TernaryExec(
	    input, offset, length, result,
	    [&](size_t input_index, size_t offset_index, size_t length_index, size_t result_index) {
		    auto input_string = input_data[input_index];
		    auto offset = offset_data[offset_index] - 1;
		    auto length = length_data[length_index];
		    assert(length <= max_str_len);

		    if (input.nullmask[input_index]) {
			    return;
		    }

		    int input_offset = 0;
		    while (input_string[input_offset] && input_offset < offset) {
			    input_offset++;
		    }
		    if (!input_string[input_offset]) {
			    // out of range, return empty string
			    output[0] = '\0';
		    } else {
			    // now limit the string
			    size_t write_offset = 0;
			    while (input_string[input_offset + write_offset] && write_offset < length) {
				    output[write_offset] = input_string[input_offset + write_offset];
				    write_offset++;
			    }
			    output[write_offset] = '\0';
		    }

		    result_data[result_index] = result.string_heap.AddString(output);
	    });
}

bool substring_matches_arguments(vector<TypeId> &arguments) {
	return arguments.size() == 3 && arguments[0] == TypeId::VARCHAR && arguments[1] == TypeId::INTEGER &&
	       arguments[2] == TypeId::INTEGER;
}

TypeId substring_get_return_type(vector<TypeId> &arguments) {
	return TypeId::VARCHAR;
}

} // namespace function
} // namespace duckdb
