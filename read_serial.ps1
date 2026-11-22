$port = New-Object System.IO.Ports.SerialPort("COM11", 115200, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$port.DtrEnable = $true
$port.RtsEnable = $true
$port.ReadTimeout = 1000
try {
    $port.Open()
    Write-Host "Port COM11 acildi, loglar okunuyor (10 saniye)..."
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt 10000) {
        try {
            $line = $port.ReadLine()
            Write-Host $line
        } catch [System.TimeoutException] {
            # continue
        }
    }
    $port.Close()
} catch {
    Write-Host "Hata: $_"
    if ($port.IsOpen) { $port.Close() }
}
