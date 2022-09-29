
<!DOCTYPE html>
<html lang="en">
    
<?php
    $servername = "localhost";
    $dBUsername = "dBUsername";
    $dBPassword = "dBPassword";
    $dBName = "dBName";
    
    $conn = mysqli_connect($servername, $dBUsername, $dBPassword, $dBName);
    
    if (!$conn) {
        die("Connection failed: ".mysqli_connect_error());
    }
    
    // Store the register address selected by the user in the DB and set the register value to
    // 256 to indicate to the UI that we are waiting for the register value to arrive from
    //  Node (via Gateway) - could take a while
    if (isset($_POST["regsel"])) {
        $post_var=$_POST["regsel"];
        //$decval=intval($post_var);
        $update = mysqli_query($conn, "UPDATE SensorBank SET int_value = $post_var WHERE id = 7;");
        // tag reg value not yet valid
        $update = mysqli_query($conn, "UPDATE SensorBank SET int_value = 256 WHERE id = 8;");
        exit;
    }	
    

    // Connect to DB and read register sent/received address and value from DB into variables
        $result   = $conn->query("SELECT * FROM SensorBank;");
        $rows  = $result->fetch_all(MYSQLI_ASSOC);// Makes field names accessible, else simple numeric index.
        foreach ($rows as $row) {
            if($row['id']==7){$txregAddr=$row['int_value'];}
            if($row['id']==8){$regValue=$row['int_value'];}
            if($row['id']==9){$rxregAddr=$row['int_value'];}
        }
?>


<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sensor dashboard</title>
    <link rel="stylesheet" href="loralink_styles.css">
    <link rel="stylesheet" href="index_styles.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.7.0/jquery.min.js" type="text/javascript"></script>
</head>

<body> 
    <div>
    <a href="index.php"><input type="submit" value="Home" id="HomeButton"></a>
    <center>
        <h2 id="header-text">LoRa link evaluation</h2>
    </center>
    <br><br>
    </div>
    <div class="flex-container" >
        <form action="/index.php" method="POST">
            <label id="regselect">Select reg:</label>
            <select name="loraregs" id="loraregs">
                <option value=0x00> REG_FIFO</option>
                <option value=0x01> REG_OP_MODE</option>
                <option value=0x06> REG_FRF_MSB</option>
                <option value=0x07> REG_FRF_MID</option>
                <option value=0x08> REG_FRF_LSB</option>
                <option value=0x09> REG_PA_CONFIG</option>
                <option value=0x0b> REG_OCP</option>
                <option value=0x0c> REG_LNA</option>
                <option value=0x0d> REG_FIFO_ADDR_PTR</option>
                <option value=0x0e> REG_FIFO_TX_BASE_ADDR</option>
                <option value=0x0f> REG_FIFO_RX_BASE_ADDR</option>
                <option value=0x10> REG_FIFO_RX_CURRENT_ADDR</option>
                <option value=0x12> REG_IRQ_FLAGS</option>
                <option value=0x13> REG_RX_NB_BYTES</option>
                <option value=0x19> REG_PKT_SNR_VALUE</option>
                <option value=0x1a> REG_PKT_RSSI_VALUE</option>
                <option value=0x1b> REG_RSSI_VALUE</option>
                <option value=0x1d> REG_MODEM_CONFIG_1</option>
                <option value=0x1e> REG_MODEM_CONFIG_2</option>
                <option value=0x20> REG_PREAMBLE_MSB</option>
                <option value=0x21> REG_PREAMBLE_LSB</option>
                <option value=0x22> REG_PAYLOAD_LENGTH</option>
                <option value=0x26> REG_MODEM_CONFIG_3</option>
                <option value=0x28> REG_FREQ_ERROR_MSB</option>
                <option value=0x29> REG_FREQ_ERROR_MID</option>
                <option value=0x2a> REG_FREQ_ERROR_LSB</option>
                <option value=0x2c> REG_RSSI_WIDEBAND</option>
                <option value=0x31> REG_DETECTION_OPTIMIZE</option>
                <option value=0x33> REG_INVERTIQ</option>
                <option value=0x37> REG_DETECTION_THRESHOLD</option>
                <option value=0x39> REG_SYNC_WORD</option>
                <option value=0x3b> REG_INVERTIQ2</option>
                <option value=0x40> REG_DIO_MAPPING_1</option>
                <option value=0x42> REG_VERSION</option>
                <option value=0x4d> REG_PA_DAC</option>
            </select>
        </form>
        <div id="reg-btn">
            <input type="submit" value="Request" onclick="GetRegSelected()">
        </div>
        <div class="resultsborder" id="resultholder">
            <div id="reg-vals">   
            Unset
            </div> 
        </div>
    
    </div>
    
<script type="text/javascript">
    function GetRegSelected() {

        var reg = document.getElementById("loraregs");
        var regaddr = reg.options[reg.selectedIndex].value;
        // alert user we are waiting for reg value to arrive
        document.getElementById("reg-vals").innerHTML = "Wait...";

        // Send requested reg to php
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "loralink.php");
        xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded"); 
        regaddr=Number(regaddr);// removes the 0x
        regaddr=regaddr.toString(10);
        var val1="regsel=";
        var val2=val1.concat(regaddr);
        xhr.send(val2);
    }
    

    // The tx regaddr is the one selected by the user. The rx regaddr is the one received from Gateway
    // Note: there is a delay in transmission from Node->Gateway->Web server
    let txAddr = "<?php echo $txregAddr ?>";
    let rxAddr = "<?php echo $rxregAddr ?>";
    let val    = "<?php echo $regValue ?>";
    let temp1 = parseInt(val, 10);
    let temp2 = (temp1).toString(16);
    let temp3 ="0x";
    let val2 = temp3.concat(temp2);

    // The currently selected reg addr at startup is always first in list.
    // if this was not the last selected reg then the value displayed will be
    // incorrect. Check the last tx reg against the current selected. If differ
    // display "Unset".
    // If value = 256 means value not yet received for register so display
    // "Wait..."
    
    // Get the current selected reg from pulldown list
    var reg = document.getElementById("loraregs");
    // remove the "0x"
    var selectedregaddr = Number(reg.options[reg.selectedIndex].value); 


    // This condition occurs at start up when reg stored in DB may not match
    // the first in pulldown list
    if( selectedregaddr != txAddr){
        document.getElementById("reg-vals").innerHTML = "Unset";
    }
    // Reg selected but vale not received yet
    else if(val=="256"){
        document.getElementById("reg-vals").innerHTML = "Wait...";
    }
    // Reg selected and received
    else if(txAddr==rxAddr){
        document.getElementById("reg-vals").innerHTML = val2;
    }


    // Refresh reg value every x ms.    
    $(document).ready(function () {
        var updater = setTimeout(function () {
            $('div#reg-vals').load('loralink.php', 'update=true');
        }, 5000);
    });


</script>
</body>


</html>