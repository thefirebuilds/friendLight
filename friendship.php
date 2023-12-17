<?php


//gather curl data
$client_data = file_get_contents("php://input");
$decodeClientData = json_decode($client_data, false);

//get hash
$verify = file_get_contents("./.hsh");

$dataSanitize = 1;
$status = $decodeClientData->status;
$node = $decodeClientData->node;
$token = $decodeClientData->token;

//data sanitization
//only allow node ella or node willy
if($node != "ella" && $node != "willy")
{ $dataSanitize = 0; }

//only allow colors from a list
//orange, red, blue, purple, white, green, magenta, yellow, 
//if($status != "ella" && $status != "willy")
//{ $dataSanitize = 0; }

if(password_verify($token, $verify) && $dataSanitize)
{
    $myfile = fopen("status.json", "w") or die("Unable to open file!");
    fwrite($myfile, build_json($status,$node,$verify));
    fclose($myfile);
}
else
{
    $myfile = fopen("status.json", "w") or die("Unable to open file!");
    fwrite($myfile, "TOKEN INVALID");
    fwrite($myfile, $client_data);
    fwrite($myfile, $token);
    fwrite($myfile, "END");
    fclose($myfile);
}

function build_json($status, $node, $verify)
{
    date_default_timezone_set('America/Chicago');
    //$date = date('m/d/Y h:i:s a', time());

    $string = "{";
    $string .= formatJSON("Current Node",$node);
    $string .= ",";
    $string .= formatJSON("Light Status",$status);
    $string .= ",";
    $string .= formatJSON("Last Update",time());
    $string .= "\n}";
    return $string;
}
function formatJSON($string,$value)
{
    $formatString = "\n\t\"" . $string . "\":\"" . $value . "\"";
    return $formatString;
}
?>
