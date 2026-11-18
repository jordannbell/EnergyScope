// Declarative Jenkins pipeline. Needs a Windows agent labelled "windows" with
// Visual Studio (C++ and MFC components) and Git installed.
//
// NOTE: written from the Jenkins documentation. It has not been run on a
// real Jenkins server; the scripts it calls (scripts/ci.ps1) were run locally.
pipeline {
    agent { label 'windows' }

    options {
        timeout(time: 30, unit: 'MINUTES')           // never hang forever
        buildDiscarder(logRotator(numToKeepStr: '20'))
        disableConcurrentBuilds()
    }

    stages {
        stage('Configure') {
            steps { powershell '.\\scripts\\ci.ps1 -Stage Configure' }
        }
        stage('Build') {
            steps { powershell '.\\scripts\\ci.ps1 -Stage Build' }
        }
        stage('Test') {
            steps { powershell '.\\scripts\\ci.ps1 -Stage Test' }
        }
    }

    post {
        always {
            // Show test results even when the Test stage failed.
            junit testResults: 'build/test-results.xml', allowEmptyResults: true
        }
        success {
            archiveArtifacts artifacts: 'build/server/energy_server.exe, build/app/energy_client.exe, build/tools/energy_export.exe',
                             fingerprint: true
        }
    }
}
