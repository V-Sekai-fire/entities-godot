/**************************************************************************/
/*  test_mcp_properties.h                                                 */
/**************************************************************************/

#pragma once

#include "../mcp_commands.h"
#include "../mcp_protocol.h"
#include "../mcp_server.h"

#include "core/io/json.h"
#include "core/math/random_pcg.h"
#include "tests/test_macros.h"

// Property tests: each one states an invariant, throws a few hundred generated
// cases at it, and carries a falsifiability control -- a deliberately wrong
// input that MUST break the same invariant. A property that has never failed
// has not been shown capable of failing.
namespace TestMCPProperties {

constexpr int CASES = 256;
constexpr uint64_t SEED = 0x9E3779B97F4A7C15ULL; // fixed, so a failure reproduces

// Generated values carry the units the commands actually move: positions and
// bounds are metres, rotations are degrees, weights are unitless 0-1.
static Variant random_value(RandomPCG &r, int p_kind) {
	switch (p_kind % 8) {
		case 0:
			return (int64_t)r.rand(1000000) - 500000;
		case 1:
			return (double)r.randf() * 100.0 - 50.0; // metres
		case 2:
			return Vector2(r.randf() * 20.0 - 10.0, r.randf() * 20.0 - 10.0); // metres
		case 3:
			return Vector3(r.randf() * 20.0 - 10.0, r.randf() * 20.0 - 10.0, r.randf() * 20.0 - 10.0); // metres
		case 4:
			return Color(r.randf(), r.randf(), r.randf(), r.randf()); // unitless 0-1
		case 5:
			return Quaternion(Vector3(0, 1, 0), r.randf() * Math::TAU); // radians about +Y
		case 6:
			return Vector3i((int)r.rand(200) - 100, (int)r.rand(200) - 100, (int)r.rand(200) - 100);
		default:
			return String::num_int64((int64_t)r.rand(100000));
	}
}

TEST_CASE("[MCP][Property] every JSON encoding decodes back to the value it came from") {
	Ref<MCPCommands> commands;
	commands.instantiate();
	RandomPCG rng(SEED);

	for (int i = 0; i < CASES; i++) {
		const Variant original = random_value(rng, i);
		const Variant encoded = commands->to_json(original);
		const Variant decoded = MCPCommands::coerce(encoded, original.get_type());
		CHECK_MESSAGE(decoded == original, vformat("round trip lost %s", String(original)));
	}

	SUBCASE("falsifiability: a tampered encoding must fail the same check") {
		// One metre moved on x. If the property cannot catch that, it cannot
		// catch a real encoding bug either.
		const Vector3 original = Vector3(1.0, 2.0, 3.0);
		Dictionary encoded = commands->to_json(original);
		encoded["x"] = 2.0;
		CHECK_FALSE(MCPCommands::coerce(encoded, Variant::VECTOR3) == Variant(original));
	}
}

TEST_CASE("[MCP][Property] a transform survives the native-JSON encoding intact") {
	Ref<MCPCommands> commands;
	commands.instantiate();
	RandomPCG rng(SEED + 1);

	for (int i = 0; i < CASES; i++) {
		// Translations in metres, spread over a room-sized volume.
		const Transform3D original = Transform3D(
				Basis(Vector3(0, 1, 0), rng.randf() * Math::TAU),
				Vector3(rng.randf() * 6.0 - 3.0, rng.randf() * 3.0, rng.randf() * 6.0 - 3.0));
		const Variant decoded = MCPCommands::coerce(commands->to_json(original), Variant::TRANSFORM3D);
		CHECK(decoded.get_type() == Variant::TRANSFORM3D);
		const Transform3D result = decoded;
		CHECK(result.origin.is_equal_approx(original.origin));
		CHECK(result.basis.is_equal_approx(original.basis));
	}

	SUBCASE("falsifiability: a shifted origin must be caught") {
		const Transform3D original;
		Transform3D moved = original;
		moved.origin.x += 0.001; // one millimetre, about a credit card's thickness
		CHECK_FALSE(moved.origin.is_equal_approx(original.origin));
	}
}

TEST_CASE("[MCP][Property] the tool registry and the dispatcher never disagree") {
	Ref<MCPCommands> commands;
	commands.instantiate();
	const Array tools = MCPProtocol::tool_schemas();

	for (int i = 0; i < tools.size(); i++) {
		const Dictionary tool = tools[i];
		const String name = tool["name"];

		// Advertised means dispatchable: no listed tool may answer "unknown cmd".
		const Dictionary result = commands->dispatch(name, Dictionary());
		const String message = result.get("msg", "");
		CHECK_MESSAGE(!message.begins_with("unknown cmd"), vformat("%s is advertised but not dispatched", name));

		// Required arguments must exist among the declared properties.
		const Dictionary schema = tool["inputSchema"];
		const Dictionary properties = schema["properties"];
		const Array required = schema.get("required", Array());
		for (int r = 0; r < required.size(); r++) {
			CHECK_MESSAGE(properties.has(required[r]), vformat("%s requires an undeclared argument", name));
		}
	}

	SUBCASE("falsifiability: a name outside the registry is rejected and not dispatched") {
		CHECK_FALSE(MCPProtocol::has_tool("definitely_not_a_tool"));
		const Dictionary result = commands->dispatch("definitely_not_a_tool", Dictionary());
		CHECK(String(result["msg"]).begins_with("unknown cmd"));
	}
}

TEST_CASE("[MCP][Property] routing answers every request with a complete response") {
	Ref<MCPServer> server;
	server.instantiate();
	RandomPCG rng(SEED + 2);

	const char *methods[] = { "GET", "POST", "PUT", "DELETE", "OPTIONS", "HEAD", "PATCH" };
	const char *paths[] = { "/mcp", "/mcp/", "/other", "/", "/mcp/extra" };

	for (int i = 0; i < CASES; i++) {
		const String method = methods[rng.rand(7)];
		const String path = paths[rng.rand(5)];
		const Dictionary out = server->route(method, path, Dictionary(), "");

		CHECK(out.has("code"));
		CHECK(out.has("ctype"));
		CHECK(out.has("body"));
		const int code = out["code"];
		CHECK(code >= 200);
		CHECK(code < 500);
		// A path outside /mcp is 404 for every method except the CORS preflight,
		// which is answered before the path is looked at.
		if (!path.begins_with("/mcp") && method != "OPTIONS") {
			CHECK(code == 404);
		}
	}

	SUBCASE("falsifiability: the 404 rule is not vacuous") {
		// It fires on a real request, and the preflight carve-out is a carve-out
		// rather than a hole: POST to an unknown path still fails the way it must.
		CHECK(int(Dictionary(server->route("POST", "/other", Dictionary(), ""))["code"]) == 404);
		CHECK_FALSE(int(Dictionary(server->route("POST", "/mcp", Dictionary(), ""))["code"]) == 404);
		CHECK(int(Dictionary(server->route("OPTIONS", "/other", Dictionary(), ""))["code"]) == 204);
	}
}

TEST_CASE("[MCP][Property] a malformed body is answered, never dropped") {
	Ref<MCPServer> server;
	server.instantiate();
	RandomPCG rng(SEED + 3);

	for (int i = 0; i < CASES; i++) {
		String body;
		const int length = rng.rand(24);
		for (int c = 0; c < length; c++) {
			body += String::chr(33 + rng.rand(90));
		}
		const Dictionary out = server->route("POST", "/mcp", Dictionary(), body);
		const int code = out["code"];
		// Either it parsed as JSON-RPC, or it came back as a parse error, but the
		// transport always answers.
		CHECK((code == 200 || code == 202));
		if (code == 200) {
			const Variant parsed = JSON::parse_string(out["body"]);
			CHECK(parsed.get_type() == Variant::DICTIONARY);
		}
	}

	SUBCASE("falsifiability: unparseable input really does produce the parse error code") {
		const Dictionary out = server->route("POST", "/mcp", Dictionary(), "{ this is not json");
		const Dictionary body = JSON::parse_string(out["body"]);
		const Dictionary error = body["error"];
		CHECK(int(error["code"]) == -32700);
	}
}

} // namespace TestMCPProperties
