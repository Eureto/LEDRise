package com.ledrise

import android.os.Bundle
import android.widget.ImageButton
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import android.content.SharedPreferences
import android.widget.EditText
import android.widget.Toast
import androidx.activity.OnBackPressedCallback

class SettingsActivity: AppCompatActivity() {

    var blinkRepetitionsInput: EditText? = null
    var ledOnMinutesInput: EditText? = null
    var backButton: ImageButton? = null

    override fun onCreate(savedInstanceState: Bundle?){
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)
        enableEdgeToEdge()
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.settings)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }

        blinkRepetitionsInput = findViewById(R.id.repetitionInput)
        ledOnMinutesInput = findViewById(R.id.LedOnMinutesInput)

            // Load saved settings from SharedPreferences
        val sharedPreferences: SharedPreferences = getSharedPreferences("AlarmSettings", MODE_PRIVATE)
        val savedBlinkRepetitions = sharedPreferences.getInt("blinkRepetitions", 0)
        val savedLedOnMinutes = sharedPreferences.getInt("ledOnMinutes", 5)
        blinkRepetitionsInput?.setText(savedBlinkRepetitions.toString())
        ledOnMinutesInput?.setText(savedLedOnMinutes.toString())


        backButton = findViewById(R.id.backButton)
        backButton?.setOnClickListener {
            saveSettingsToSharedPreferences()
            finish()
        }

        // When user click back button on device
        val onBackPressedCallback = object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                saveSettingsToSharedPreferences()
                finish()
            }
        }
        onBackPressedDispatcher.addCallback(this, onBackPressedCallback)
    }

    fun saveSettingsToSharedPreferences(){
        val sharedPreferences: SharedPreferences = getSharedPreferences("AlarmSettings", MODE_PRIVATE)
        val editor: SharedPreferences.Editor = sharedPreferences.edit()

        val blinkRepetitions = blinkRepetitionsInput?.text.toString().toIntOrNull() ?: 0 // Default to 3 if invalid
        val ledOnMinutes = ledOnMinutesInput?.text.toString().toIntOrNull() ?: 5 // Default to 60 if invalid

        editor.putInt("blinkRepetitions", blinkRepetitions)
        editor.putInt("ledOnMinutes", ledOnMinutes)
        editor.apply()

        Toast.makeText(this, "Settings saved", Toast.LENGTH_SHORT).show()
    }

}