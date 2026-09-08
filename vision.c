typedef int message_t;

enum calculator_messages {
	CALCULATOR_ADD,
	CALCULATOR_SUB,
	CALCULATOR_MUL,
	CALCULATOR_DIVREM,
};

enum {
	INVALID_PARAMETERS = -1;
};

#define REQUIRE(condition, err) do { if (!(condition)) { return err; } } while (0)

message_t handle_calculator_add(message_t info) {
	long acc = 0;
	for (int i = 0; i < info.parameters; i++)
	  acc += get_mr(i);
	set_mr(0, acc);

	return msg_ok(1);
}

message_t handle_calculator_sub(message_t info) {
	REQUIRE(info >= 1, INVALID_PARAMETERS);

	long acc = get_mr(0);
	for (int i = 1; i < info.parameters; i++)
	  acc -= get_mr(i);
	set_mr(0, acc);

	return msg_ok(1);
}

message_t handle_calculator_mul(message_t info) {
	long acc = 1;
	for (int i = 0; i < info.parameters; i++)
	  acc *= get_mr(i);
	set_mr(0, acc);

	return msg_ok(1);
}

message_t handle_calculator_divrem(message_t info) {
	REQUIRE(info == 2, INVALID_PARAMETERS);

	long a = get_mr(0);
	long b = get_mr(1);

	set_mr(0, a/b);
	set_mr(1, a%b);

	return msg_ok(2);
}


typedef message_t message_handler_fn(message_t);


const auto handlers = (message_handler_fn[]) {
	[CALCULATOR_ADD] = handle_caldulator_add,
	[CALCULATOR_SUB] = handle_caldulator_sub,
	[CALCULATOR_MUL] = handle_caldulator_mul,
	[CALCULATOR_DIVREM] = handle_caldulator_divrem,
};

int main() {
	run_service("calculator", supervisor_ep, handlers, countof(handlers));
}


set_mr(0, 1);
set_mr(1, 1);
info = TRY(call_service(calculator_ep, 2));

