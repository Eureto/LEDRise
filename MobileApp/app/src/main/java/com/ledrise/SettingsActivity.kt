package com.ledrise

import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.ImageButton
import android.widget.TextView
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch

/** Settings live on the ESP32 (survive reboots); this screen only reads and writes them. */
class SettingsActivity : AppCompatActivity() {

    private lateinit var api: DeviceApi
    private lateinit var blinkRepetitionsInput: EditText
    private lateinit var ledOnMinutesInput: EditText
    private lateinit var deviceText: TextView
    private var loaded = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)
        enableEdgeToEdge()
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.settings)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        api = DeviceApi.get(this)

        blinkRepetitionsInput = findViewById(R.id.repetitionInput)
        ledOnMinutesInput = findViewById(R.id.LedOnMinutesInput)
        deviceText = findViewById(R.id.deviceText)

        findViewById<ImageButton>(R.id.backButton).setOnClickListener { saveAndFinish() }
        findViewById<Button>(R.id.buttonFindDevice).setOnClickListener { loadFromDevice(forceScan = true) }
        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() = saveAndFinish()
        })

        loadFromDevice(forceScan = false)
    }

    private fun loadFromDevice(forceScan: Boolean) {
        deviceText.text = "Searching for device..."
        lifecycleScope.launch {
            try {
                if (forceScan) api.discover()
                val info = api.getInfo()
                blinkRepetitionsInput.setText(info.flashingRepetitions.toString())
                ledOnMinutesInput.setText(info.ledOnMinutes.toString())
                deviceText.text = "Device: ${api.host}"
                loaded = true
            } catch (e: Exception) {
                deviceText.text = "Device not found in this network"
            }
        }
    }

    private fun saveAndFinish() {
        if (!loaded) {
            finish()
            return
        }
        val blink = (blinkRepetitionsInput.text.toString().toIntOrNull() ?: 0).coerceIn(0, 10000)
        val minutes = (ledOnMinutesInput.text.toString().toIntOrNull() ?: 5).coerceIn(0, 60)
        lifecycleScope.launch {
            try {
                api.setLedOnMinutes(minutes)
                api.setFlashing(blink)
                Toast.makeText(this@SettingsActivity, "Settings saved", Toast.LENGTH_SHORT).show()
            } catch (e: Exception) {
                Toast.makeText(this@SettingsActivity, "Could not save settings: device not reachable", Toast.LENGTH_SHORT).show()
            }
            finish()
        }
    }
}
