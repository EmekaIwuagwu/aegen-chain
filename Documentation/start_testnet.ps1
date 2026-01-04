$nodePath = ".\build\Release\aegen-node.exe"

# Clean up old data
if (Test-Path "data_node1") { Remove-Item -Recurse -Force "data_node1" }
if (Test-Path "data_node2") { Remove-Item -Recurse -Force "data_node2" }
if (Test-Path "data_node3") { Remove-Item -Recurse -Force "data_node3" }

# Start Node 1
Write-Host "Starting Node 1..."
Start-Process -FilePath $nodePath -ArgumentList "--node node-1 --rpc 8545 --p2p 30303 --peers localhost:30304,localhost:30305 --data data_node1" -WindowStyle Normal

# Start Node 2
Write-Host "Starting Node 2..."
Start-Process -FilePath $nodePath -ArgumentList "--node node-2 --rpc 8546 --p2p 30304 --peers localhost:30303,localhost:30305 --data data_node2" -WindowStyle Normal

# Start Node 3
Write-Host "Starting Node 3..."
Start-Process -FilePath $nodePath -ArgumentList "--node node-3 --rpc 8547 --p2p 30305 --peers localhost:30303,localhost:30304 --data data_node3" -WindowStyle Normal

Write-Host "Testnet started with 3 nodes."
