<?php
    $servername = "localhost";
    $dBUsername = "dBUsername";
    $dBPassword = "dBPassword";
    $dBName = "dBName";


$conn = mysqli_connect($servername, $dBUsername, $dBPassword, $dBName);
if (!$conn) {
	die("Connection failed: ".mysqli_connect_error());
}

// Gateway requests LED status ON/OFF
// Results is echoed back to Gateway
if (isset($_POST['check_LED_status'])) {
	$led_id = $_POST['check_LED_status'];	
	$sql = "SELECT * FROM SensorBank WHERE id = 5;";
	$result   = mysqli_query($conn, $sql);
	$row  = mysqli_fetch_assoc($result);
	if($row['int_value'] == 0){
		echo "LED_is_off";
	}
	else{
		echo "LED_is_on";
	}	
}	


// Update the database with adc value sent from Gateway
// Echoed back to Gateway for confirmation
if (isset($_POST['Adc_Data'])) {
	$adcValue = $_POST['Adc_Data'];	
	$sql = "SELECT * FROM SensorBank WHERE id = 1;";
	$result   = mysqli_query($conn, $sql);
	$row  = mysqli_fetch_assoc($result);
	$update = mysqli_query($conn, "UPDATE SensorBank SET int_value = $adcValue WHERE id = 1;");
	echo "Adc_data_received = $adcValue";
}

//Update the database with humidity value sent from Gateway
// Echoed back to Gateway for confirmation
if (isset($_POST['Hum_Data'])) {
	$humValue = $_POST['Hum_Data'];	
	$sql = "SELECT * FROM SensorBank WHERE id = 2;";
	$result   = mysqli_query($conn, $sql);
	$row  = mysqli_fetch_assoc($result);
	$update = mysqli_query($conn, "UPDATE SensorBank SET float_value = $humValue WHERE id = 2;");
	echo "Hum_data_received = $humValue";
}

//Update the database with temp C value sent from Gateway
// Echoed back to Gateway for confirmation
if (isset($_POST['Tempc_Data'])) {
	$tempcValue = $_POST['Tempc_Data'];	
	$sql = "SELECT * FROM SensorBank WHERE id = 3;";
	$result   = mysqli_query($conn, $sql);
	$row  = mysqli_fetch_assoc($result);
	$update = mysqli_query($conn, "UPDATE SensorBank SET float_value = $tempcValue WHERE id = 3;");
	echo "Tempc_data_received = $tempcValue";
}

//Update the database with temp F value sent from Gatway
// Echoed back to Gateway for confirmation
if (isset($_POST['Tempf_Data'])) {
	$tempfValue = $_POST['Tempf_Data'];	
	$sql = "SELECT * FROM SensorBank WHERE id = 4;";
	$result   = mysqli_query($conn, $sql);
	$row  = mysqli_fetch_assoc($result);
	$update = mysqli_query($conn, "UPDATE SensorBank SET float_value = $tempfValue WHERE id = 4;");
	echo "Tempf_data_received = $tempfValue";
}


//Update the database with the RSSI of the LoRa link sent from Gatway
// Echoed back to Gateway for confirmation
if (isset($_POST['Rssi_Data'])) {
	$rssiValue = $_POST['Rssi_Data'];	
	$sql = "SELECT * FROM SensorBank WHERE id = 6;";
	$result   = mysqli_query($conn, $sql);
	$row  = mysqli_fetch_assoc($result);
	$update = mysqli_query($conn, "UPDATE SensorBank SET int_value = $rssiValue WHERE id = 6;");
	echo "RSSI_received = $rssiValue";
}

// Gateway requests LoRa register address selected by the user
// Register address echoed back to Gateway
if (isset($_POST['check_lora_reg'])) {
	$sql = "SELECT * FROM SensorBank WHERE id = 7;";
	$result   = mysqli_query($conn, $sql);
	$row  = mysqli_fetch_assoc($result);
	$regAddr = $row['int_value'];
	echo "reg_selected=$regAddr";
}

// Gateway sends reguster value (from Node) along with actual register address so that
// transaction remains in sync.
// DB is updated with register value and RX register address
if (isset($_POST['Lora_reg_value'])) {
	$regValue = $_POST['Lora_reg_value'];	
	$sql = "SELECT * FROM SensorBank WHERE id = 8;";
	$result   = mysqli_query($conn, $sql);
	$row  = mysqli_fetch_assoc($result);
	$update = mysqli_query($conn, "UPDATE SensorBank SET int_value = $regValue WHERE id = 8;");
    if (isset($_POST['Lora_reg_addr'])) {
    	$regAddr = $_POST['Lora_reg_addr'];	
    	$sql = "SELECT * FROM SensorBank WHERE id = 9;";
    	$result   = mysqli_query($conn, $sql);
    	$row  = mysqli_fetch_assoc($result);
    	$update = mysqli_query($conn, "UPDATE SensorBank SET int_value = $regAddr WHERE id = 9;");

	    echo "Lora_reg_value_received = $regValue AND Lora_reg_addr_received = $regAddr";
    }    
}	


?>