<!DOCTYPE html>
<html lang="en">
<?php
    $servername = "localhost";
    $dBUsername = "dbUsername";
    $dBPassword = "dbPassword";
    $dBName = "dbName";

    $conn = mysqli_connect($servername, $dBUsername, $dBPassword, $dBName);
    
    if (!$conn) {
        die("Connection failed: ".mysqli_connect_error());
    }
    
    // When the switch is activated on the browser the state is sent to the server which is then stored in the DB
    if (isset($_POST["ledswitch"])) {
        $post_var=$_POST["ledswitch"];
        if($post_var== "1"){
            $update = mysqli_query($conn, "UPDATE SensorBank SET int_value = 1 WHERE id = 5;");
        }
        else{
            $update = mysqli_query($conn, "UPDATE SensorBank SET int_value = 0 WHERE id = 5;");
        }
        exit; // Stops entire file being returned 
    }	
    

    // Connect to DB and read sensor values into php variables
        $result   = $conn->query("SELECT * FROM SensorBank;");
        $rows  = $result->fetch_all(MYSQLI_ASSOC);// Makes field names accessible, else simple numeric index.
        foreach ($rows as $row) {
            if($row['id']==1){$adcValue=$row['int_value'];}
            if($row['id']==2){$humValue=$row['float_value'];}
            if($row['id']==3){$tempcValue=$row['float_value'];}
            if($row['id']==4){$tempfValue=$row['float_value'];}
            if($row['id']==5){$ledStatus=$row['int_value'];}
            if($row['id']==6){$rssiValue=$row['int_value'];}
        }
    ?>

<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sensor dashboard</title>
    <link rel="stylesheet" href="index_styles.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.7.0/jquery.min.js" type="text/javascript"></script>
</head>

<body>
    <div id="refresh">
        <center>
            <h2 id="header-text">remote sensor dashboard</h2>
            <!-- sensor name labels and value placeholders are cretaed within a grid -->
            <div id="grid-container">
                <div class="item s1-name">Voltage</div>
                <div class="item s2-name">Humidity %</div>
                <div class="item s3-name">Temperature °C</div>
                <div class="item s4-name">Temperature °F</div>
                <div class="item s5-name">LED state</div>
                <div class="item s6-name">RSSI</div>

                <div class="item s1-data">
                    <p class="resultsborder" id="sval-1"></p>
                </div>
                <div class="item s2-data">
                    <p class="resultsborder" id="sval-2"></p>
                </div>
                <div class="item s3-data">
                    <p class="resultsborder" id="sval-3"></p>
                </div>
                <div class="item s4-data">
                    <p class="resultsborder" id="sval-4"></p>
                </div>
                <div class="switch-item s5-data">
                    <input type="checkbox" id="switch" class="checkbox" onchange="switchChange()" />
                    <label for="switch" class="toggle" name="ledswitch">
                        <div>
                            <span class="off">OFF</span>
                            <span class="on">ON</span>
                        </div>
                    </label>
                </div>
                <div class="item s6-data">
                    <p class="resultsborder" id="sval-6"></p>
                </div>
                <div class="item s1-next"></div>
                <div class="item s2-next"></div>
                <div class="item s3-next"></div>
                <div class="item s4-next"></div>
                <div class="item s5-next"></div>
                <div class="item s6-next"></div>
                <div class="item link-button-grid"><a href="loralink.php"><input type="submit" value="Node LoRa" id="LinkButton"></a></div>
            </div>

            <script type="text/javascript">
                // Load sensor values to webpage

                // Initially set slider switch to state stored in DB
                let val = "<?php echo $ledStatus ?>";
                if (val == 1)
                    document.getElementById("switch").checked = true;
                else
                    document.getElementById("switch").checked = false;


                function switchChange() {
                    $(document).ready(function () {
                        // Send switch state to php
                        const xhr = new XMLHttpRequest();
                        xhr.open("POST", "index.php");
                        xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
                        if (document.getElementById("switch").checked == true)
                            xhr.send("ledswitch=1");
                        else
                            xhr.send("ledswitch=0");
                    });
                }// SwitchChange

                // Write sensor values in their placeholders
                val = "<?php echo $adcValue ?>";
                document.getElementById("sval-1").innerHTML = val;
                val = "<?php echo $humValue ?>";
                document.getElementById("sval-2").innerHTML = val;
                val = "<?php echo $tempcValue ?>";
                document.getElementById("sval-3").innerHTML = val;
                val = "<?php echo $tempfValue ?>";
                document.getElementById("sval-4").innerHTML = val;

                // Set RSSI value for LoRa link
                val = "<?php echo $rssiValue ?>";
                document.getElementById("sval-6").innerHTML = val;


                // Add any new sensor display code here...

                // Refresh the webpage from div/refresh every x ms.
                $(document).ready(function () {
                    var updater = setTimeout(function () {
                        $('div#refresh').load('index.php', 'update=true');
                    }, 5000);
                });

            </script>
        </center>

    </div>
</body>

</html>