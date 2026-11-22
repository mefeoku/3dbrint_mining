$port = New-Object System.IO.Ports.SerialPort("COM11", 115200, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$port.DtrEnable = $true
$port.RtsEnable = $false
$port.ReadTimeout = 1000
try {
    $port.Open()
    Write-Host "COM11 dinleniyor (10 saniye)..."
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt 10000) {
        try {
            $line = $port.ReadLine()
            Write-Host $line
        } catch [System.TimeoutException] {}
    }
    $port.Close()
} catch {
    Write-Host "Hata: $_"
    if ($port.IsOpen) { $port.Close() }
}
