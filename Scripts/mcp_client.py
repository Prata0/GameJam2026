"""Minimal client for the Unreal editor's MCP server (streamable HTTP), used by the testbed scripts.

The editor must be open with the Unreal MCP plugin listening on 127.0.0.1:8003 (see .mcp.json).

    python mcp_client.py <method> [json params]                 e.g.  python mcp_client.py tools/list
    python mcp_client.py call <toolset> <tool> [json arguments]
"""
import json
import sys
import urllib.request

URL = "http://127.0.0.1:8003/mcp"
_session = None
_next_id = 0


def _post(payload):
    global _session
    headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
    if _session:
        headers["Mcp-Session-Id"] = _session
    request_ = urllib.request.Request(URL, data=json.dumps(payload).encode(), headers=headers, method="POST")
    with urllib.request.urlopen(request_, timeout=300) as response:
        _session = response.headers.get("Mcp-Session-Id") or _session
        body = response.read().decode("utf-8")
    if not body.strip():
        return None
    if body.lstrip().startswith("{"):
        return json.loads(body)
    result = None
    for line in body.splitlines():
        if line.startswith("data:"):
            result = json.loads(line[5:].strip())
    return result


def request(method, params=None):
    global _next_id
    _next_id += 1
    return _post({"jsonrpc": "2.0", "id": _next_id, "method": method, "params": params or {}})


def init():
    request("initialize", {"protocolVersion": "2025-03-26", "capabilities": {},
                           "clientInfo": {"name": "stdcore-testbed", "version": "1"}})
    _post({"jsonrpc": "2.0", "method": "notifications/initialized"})


def call(toolset, tool, arguments=None):
    params = {"toolset_name": toolset, "tool_name": tool}
    if arguments is not None:
        params["arguments"] = arguments
    return request("tools/call", {"name": "call_tool", "arguments": params})


def text_of(response):
    """The tool's text output; errors start with ERROR or TOOL ERROR."""
    if not response:
        return ""
    if "error" in response:
        return "ERROR " + json.dumps(response["error"])
    content = response.get("result", {}).get("content", [])
    out = "\n".join(c.get("text", "") for c in content)
    if response.get("result", {}).get("isError"):
        out = "TOOL ERROR " + out
    return out


def result(response):
    """The tool's returnValue; raises on errors."""
    out = text_of(response)
    if out.startswith("TOOL ERROR") or out.startswith("ERROR"):
        raise RuntimeError(out)
    return json.loads(out)["returnValue"] if out else None


if __name__ == "__main__":
    init()
    if sys.argv[1] == "call":
        arguments = json.loads(sys.argv[4]) if len(sys.argv) > 4 else None
        print(text_of(call(sys.argv[2], sys.argv[3], arguments)))
    else:
        params = json.loads(sys.argv[2]) if len(sys.argv) > 2 else None
        print(json.dumps(request(sys.argv[1], params), indent=1))
