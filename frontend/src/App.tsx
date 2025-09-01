import { Dashboard } from './components';
import './index.css';

function App() {
  return (
    <div className="App">
      <Dashboard refreshInterval={1000} theme="dark" />
    </div>
  );
}

export default App;