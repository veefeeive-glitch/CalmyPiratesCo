param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Toolset = 'EditorToolset.EditorAppToolset',
    [string]$ArgumentsJson = '{}',
    [string]$OutputPng
)
# Direct client for the existing official Unreal MCP endpoint, useful after an editor restart.
$ErrorActionPreference = 'Stop'
$uri = 'http://127.0.0.1:8000/mcp'
$headers = @{Accept='application/json, text/event-stream'; 'Content-Type'='application/json'}
$response = Invoke-WebRequest -Uri $uri -Method Post -Headers $headers -Body '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"calmy-local-tools","version":"1"}}}'
$headers['Mcp-Session-Id'] = [string](@($response.Headers['Mcp-Session-Id'])[0])
Invoke-WebRequest -Uri $uri -Method Post -Headers $headers -Body '{"jsonrpc":"2.0","method":"notifications/initialized"}' | Out-Null
$request = @{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name=$Toolset;tool_name=$Name;arguments=($ArgumentsJson | ConvertFrom-Json)}}}
$response = Invoke-WebRequest -Uri $uri -Method Post -Headers $headers -TimeoutSec 60 -Body ($request | ConvertTo-Json -Depth 40 -Compress)
$body = $response.Content | ConvertFrom-Json
if ($body.error -or $body.result.isError) { throw ($body | ConvertTo-Json -Depth 10) }
if ($OutputPng) {
    $img = @($body.result.content | Where-Object type -eq image)[0]
    if (-not $img) {
        $value = ($body.result.content | Where-Object type -eq text | Select-Object -First 1).text | ConvertFrom-Json
        $img = $value.returnValue.image
        if (-not $img) { $img = $value.returnValue }
    }
    if (-not $img.data) { throw 'MCP returned no image.' }
    if (Test-Path -LiteralPath $OutputPng) { throw 'Refusing to overwrite an existing capture.' }
    [IO.File]::WriteAllBytes($OutputPng, [Convert]::FromBase64String($img.data))
    Write-Output $OutputPng
} else {
    $body.result | ConvertTo-Json -Depth 40
}
