#requires -Version 7.0
# Read-only handshake/discovery check. Does not execute tools or modify a map.
$ErrorActionPreference = 'Stop'
$uri = 'http://127.0.0.1:8000/mcp'
$headers = @{Accept='application/json, text/event-stream'}
function Request-Mcp($body) {
    Invoke-WebRequest -Uri $uri -Method Post -ContentType 'application/json' -Headers $headers -Body ($body | ConvertTo-Json -Depth 12) -TimeoutSec 20
}
$init = Request-Mcp @{jsonrpc='2.0';id=1;method='initialize';params=@{
    protocolVersion='2025-11-25';capabilities=@{};clientInfo=@{name='CalmySetupCheck';version='1.0'}
}}
$negotiated = ($init.Content | ConvertFrom-Json).result.protocolVersion
if (-not $negotiated) { throw 'MCP initialization did not negotiate a version' }
$headers['MCP-Protocol-Version'] = $negotiated
if ($init.Headers['Mcp-Session-Id']) {
    $headers['Mcp-Session-Id'] = [string]($init.Headers['Mcp-Session-Id'] | Select-Object -First 1)
}
try {
    $null = Request-Mcp @{jsonrpc='2.0';method='notifications/initialized'}
    $response = Request-Mcp @{jsonrpc='2.0';id=2;method='tools/list'}
    $data = $response.Content | ConvertFrom-Json
    if ($data.error -or -not $data.result.tools.Count) { throw 'MCP tool discovery failed' }
    [pscustomobject]@{result='UNREAL_MCP_DISCOVERY_PASS';protocol=$negotiated;tools=@($data.result.tools.name);gameplayTest=$false} | ConvertTo-Json -Depth 4
} finally {
    if ($headers.ContainsKey('Mcp-Session-Id')) {
        try { $null = Invoke-WebRequest -Uri $uri -Method Delete -Headers $headers -TimeoutSec 10 }
        catch { Write-Warning 'Probe session cleanup was not accepted; no editor was stopped.' }
    }
}
