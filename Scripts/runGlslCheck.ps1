param($file, $output)
    
$out = & $env:VULKAN_SDK\bin\glslangValidator.exe -C -V $file -o $output

if ($LastExitCode -eq 0)
{
	Exit 0
}
else
{
    $first = $true
	$out | ForEach-Object {
		if ($_ -Match "No code generated")
		{
			break;
		}
        if ($first)
        {
            $first = $false
        }
        else
        {
#            $_
            $str = $_ -replace "([a-z ]+):.+:([0-9]+):(.*)", '($2) : $1 V0 : $3'
	    $file + $str
        }
	}
	exit 2
}
# SIG # Begin signature block
# MIIFdgYJKoZIhvcNAQcCoIIFZzCCBWMCAQExCzAJBgUrDgMCGgUAMGkGCisGAQQB
# gjcCAQSgWzBZMDQGCisGAQQBgjcCAR4wJgIDAQAABBAfzDtgWUsITrck0sYpfvNR
# AgEAAgEAAgEAAgEAAgEAMCEwCQYFKw4DAhoFAAQU9G4jhiS1PNfzgJD9FciDnmeB
# 31WgggMOMIIDCjCCAfKgAwIBAgIQJ7QhgRtobKJFyrX3zvZHpjANBgkqhkiG9w0B
# AQUFADAdMRswGQYDVQQDDBJMb2NhbCBDb2RlIFNpZ25pbmcwHhcNMjAwOTAyMTUw
# NjQ0WhcNMjEwOTAyMTUyNjQ0WjAdMRswGQYDVQQDDBJMb2NhbCBDb2RlIFNpZ25p
# bmcwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDZgckf5EM3ekbD9UHM
# uPAb3n+ruyIkD5y/uRfKbjQ9R6hpjzz5MK8qD9EOPn1rX9QUy1QJyuGQENCbOAot
# k9UE6NUtdv6cOT+4EHtcyfpW03l1LWaBpplS+4CBddfig03rpyaqQuVJe93WQ4Ar
# eUAvWozM9/Smt39MxwsyneTAHWzPFJnBXBry3OfGqg7sfJdFh02+KDOfgBfDUrGT
# dFiXzwPyZt1MfuHbQUvd+o1ErneWSqKXFjnxP/8va7mSLFjXEDZqLhm+p96jlGEs
# 0nNSLf7AfG/ZMTKfMY/X77fd08U1Dy//xc/e4qkraQfiN5TJCz8im/E6WFzrF1eQ
# XRJFAgMBAAGjRjBEMA4GA1UdDwEB/wQEAwIHgDATBgNVHSUEDDAKBggrBgEFBQcD
# AzAdBgNVHQ4EFgQU9nmxQrbevGH/Lmr0GxeZa9LIwJ0wDQYJKoZIhvcNAQEFBQAD
# ggEBAC4XGOmt8kosp5Wdu5FbVmLUanHNUcA0S46UyEyNK8yBDnGiNJf0hVG5D/Jh
# w4mk3BuKZjFcsV5bA1ZKcmE6hrUFxmGGfgnFmEGmdI+Tt9vR0bv3re7SG4jlaMnu
# VgOEjn+AIpon8djn10Warb6q45Q28O+3lWP0D832Sx69sI06QNbpzB88QQZJNgBD
# 29AmDvESdzCyCRH1aJAN93AgjwmXkTzgfnWL3N6myKYRMmSfShdJD2s/GxoB0hZy
# 1OleXmsYOEZP9KUkqo5p7eW0Koxq0EdzTmuvJSUeRr7MT+PpVrp/vaKYRg/DTGxW
# jcTmxfUgz2AjWi+8UcjXZAjw95kxggHSMIIBzgIBATAxMB0xGzAZBgNVBAMMEkxv
# Y2FsIENvZGUgU2lnbmluZwIQJ7QhgRtobKJFyrX3zvZHpjAJBgUrDgMCGgUAoHgw
# GAYKKwYBBAGCNwIBDDEKMAigAoAAoQKAADAZBgkqhkiG9w0BCQMxDAYKKwYBBAGC
# NwIBBDAcBgorBgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAjBgkqhkiG9w0BCQQx
# FgQUGs1bA3nB8BWLAwVuUIdDzYmeyVowDQYJKoZIhvcNAQEBBQAEggEAywOum4p6
# fa+R0Jto1tnfRKBKL+vu02ZIAIL7gEjEAHppqYno06N4qoepwpNKP0bkytOAkHIW
# XbB7xrxClhD4/+wEkcUsithSx6lESEpHORA0/hnpyB5gywOb1IZz6ETi883VpLG9
# h2VAtiDvaykUogH1htLDnlDl7TDQ4Q1g3NKUeDNIfxICwOALhf0IJDfLYItOW+6G
# qZz1q1TGDgG8FJCKAWHpntVSepBfCAYi2LH2jBI6qC+5lAq/+frfzKQUYIoj58Ur
# wTKXLG87C+J/XwINAug/5S049G+k1RmPkywCboc2+GPe7ahtZLlp0xqjFXLbhbUd
# 1q0+yt5UsO5/nA==
# SIG # End signature block
