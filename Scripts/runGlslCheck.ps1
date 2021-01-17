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