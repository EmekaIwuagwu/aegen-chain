$target = "k:46010cccab74f4190e12d42251fd0e364b15699fe135d1fac6a5ae6bf129a643"
$amount = "1000"
$nodeUrl = "http://138.68.65.46:8545/"

Write-Host "Fetching Alice's nonce from $nodeUrl..."

# 1. Get Nonce
$nonceBody = @{
    jsonrpc = "2.0"
    method = "getNonce"
    params = @{
        account = "alice"
    }
    id = 1
} | ConvertTo-Json

try {
    $nonceRes = Invoke-RestMethod -Uri $nodeUrl -Method Post -Body $nonceBody -ContentType "application/json"
    $nonce = $nonceRes.result
    Write-Host "Alice's current nonce: $nonce" -ForegroundColor Cyan
} catch {
    Write-Host "Failed to get nonce. Defaulting to 0." -ForegroundColor Yellow
    $nonce = "0"
}

# 2. Send Transaction
Write-Host "Sending $amount AE to $target..."

$txBody = @{
    jsonrpc = "2.0"
    method = "sendTransaction"
    params = @{
        sender = "alice"
        receiver = $target
        amount = $amount
        nonce = "$nonce"
    }
    id = 2
} | ConvertTo-Json

try {
    $response = Invoke-RestMethod -Uri $nodeUrl -Method Post -Body $txBody -ContentType "application/json"
    
    if ($response.error) {
        Write-Host "RPC Error: $($response.error.message)" -ForegroundColor Red
    } else {
        Write-Host "Success! Transaction sent." -ForegroundColor Green
        Write-Host "Request Key: $($response.result.requestKey)" -ForegroundColor Cyan
    }
} catch {
    Write-Host "Error sending transaction: $_" -ForegroundColor Red
}
